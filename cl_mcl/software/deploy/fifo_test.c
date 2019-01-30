#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "deploy.h"
#include "../device.h"
#include "fifo.h"

static const int DW_PER_FSB = 4;

bool test_fifos();
void loop_test(int n, bool (*f) ());

int main () {
	
	printf("Running fifo tests.\n\n");

	/* Setup host */
	struct Host *host = (struct Host *) malloc(sizeof(struct Host));	 
	deploy_init_host(host, 0, 0); // DMA arguments unused	

	/* mmap the OCL BAR */
	char *ocl_base = deploy_mmap_ocl();
	if (ocl_base == 0) {
		printf("Error when mmap'ing OCL Bar.\n");
		return 0;
	}

	loop_test(1, test_fifos);

	return 0;
}

/* calls test function f() n times. */
void loop_test (int n, bool (*f) () ) {
	for (int i = 0; i < n; i++) {
		bool pass = f();
		if (!pass) {
			printf("Test %d failed.\n", i);
			return;
		}
	}
	printf("All tests pass.\n");
}

bool test_fifos () {
	printf("Running test.\n");
	int *write, *read, num_fsb;
	num_fsb = 1;
	write = calloc(num_fsb * NUM_FIFO * DW_PER_FSB, sizeof(int)); 	
	/*----------------------------------------------------------------------------*/
	/* POWER UP FIFOs */
	for (int f = 0; f < NUM_FIFO; f++) {
		uint32_t isr = ocl_read(fifo[f][FIFO_ISR]); /* verify that fifos are reset */
		if (!is_trc(isr) || !is_rrc(isr)) {
			if (!is_trc(isr))
				ocl_write(fifo[f][FIFO_TDFR], 0x000000A5);
			if (!is_rrc(isr))
				ocl_write(fifo[f][FIFO_RDFR], 0x000000A5);
			if (!is_trc(isr) || !is_rrc(isr)) {
				ocl_write(fifo[f][FIFO_SRR], 0x000000A5);
					if (!is_trc(isr) || !is_rrc(isr)) {
						printf("FIFO %d: Unable to reset Transmit and Receive FIFOs.\n", f);
						return false;
					}
			}
		}
		
		clear_int(f);	/* clear all interrupts */	
		if (ocl_read(fifo[f][FIFO_ISR])) { /* verify that no interrupts are pending */ 
			printf("FIFO %d: There are pending interrupts for unknown reasons.\n", f);
			return false;
		}
		
		if (ocl_read(fifo[f][FIFO_IER])) { /* disable interrupts */
			ocl_write(fifo[f][FIFO_IER], 0); 	
			if (ocl_read(fifo[f][FIFO_IER])) {/* verify that interrupts have been disabled */
				printf("FIFO %d: Unable to disable interrupts.\n", f);
				return false;
			}
		}
	
		if (ocl_read(fifo[f][FIFO_VACANCY]) != 508 || ocl_read(fifo[f][FIFO_OCCUPANCY]) != 0) {
			printf("FIFO %d: Unexpected (vacancy, occupancy).\n");
			return false;
		}	
	}
	printf("Power up complete!\n");
	/*----------------------------------------------------------------------------*/
	/* set destination */
	for (int f = 0; f < NUM_FIFO; f++) {
		if (!set_dest(f))
			return false;
	}	
	/*----------------------------------------------------------------------------*/
	/* WRITE then READ TEST */
	for (int pkt = 0; pkt < num_fsb; pkt++) {
		/* write to fifos */
		for (int f = 0; f < NUM_FIFO; f++) {
			for(int j = 0; j < 4; j++) { /* make write packet */
				write[(pkt * NUM_FIFO * DW_PER_FSB)+ (f * DW_PER_FSB) + j] = rand();
			}
			deploy_write_fifo(f, write + (pkt * NUM_FIFO * DW_PER_FSB) + (f * DW_PER_FSB)); /* write the packets */
		}

		usleep(100);
		
		/* read from fifos  */
		bool pass = true;
		for (int f = 0; f < NUM_FIFO; f++) {
			read = deploy_read_fifo(f, NULL);
			if (!read) {
				printf("No read data for fsb packet %d received from fifo %d.\n", pkt, f);
				pass = false;
			}
			else {
				for (int j = 0; j < 2; j++) {
					if (read[j] != (write + (pkt * NUM_FIFO * DW_PER_FSB) + (f * DW_PER_FSB))[j]) {
						printf("Mismatch at fsb packet %d, DW %d from fifo %d. Received %d instead of %x. \n", pkt, j, f, j, read[j], j);
						pass = false;
						break;
					}
				}
			}
			if (read)
				free(read);

			// sleep(1);
		}
		if (pass) {
			printf("packet %d passed.\n", pkt) ;
		}
	}
	/*----------------------------------------------------------------------------*/
	free(write);
	return true;
}





