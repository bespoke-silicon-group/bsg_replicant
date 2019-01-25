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

void test_fifos();
void loop_test(int n, void (*f) ());

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

	loop_test(50, test_fifos);

	return 0;
}

/* calls test function f() n times. */
void loop_test (int n, void (*f) () ) {
	for (int i = 0; i < n; i++) 
		f();
}
void test_fifos () {
	int *write, *read, num_fsb;
	num_fsb = 10;
	write = calloc(num_fsb * NUM_FIFO * DW_PER_FSB, sizeof(int)); 	

	/* clear fifo interrupts */
	for (int fifo = 0; fifo < NUM_FIFO; fifo++)
		clear_int(fifo);

	for (int pkt = 0; pkt < num_fsb; pkt++) {
		/* write to fifos */
		for (int fifo = 0; fifo < NUM_FIFO; fifo++) {
			for(int j = 0; j < 4; j++) { /* make write packet */
				write[(pkt * NUM_FIFO * DW_PER_FSB)+ (fifo * DW_PER_FSB) + j] = rand();
			}
			deploy_write_fifo(fifo, write + (pkt * NUM_FIFO * DW_PER_FSB) + (fifo * DW_PER_FSB)); /* write the packets */
		}

		usleep(100);
		
		/* read from fifos  */
		bool pass = true;
		for (int fifo = 0; fifo < NUM_FIFO; fifo++) {
			read = deploy_read_fifo(fifo, NULL);
			if (!read) {
				printf("No read data for fsb packet %d received from fifo %d.\n", pkt, fifo);
				pass = false;
			}
			else {
				for (int j = 0; j < 2; j++) {
					if (read[j] != (write + (pkt * NUM_FIFO * DW_PER_FSB) + (fifo * DW_PER_FSB))[j]) {
						printf("Mismatch at fsb packet %d, DW %d from fifo %d. Received %d instead of %x. \n", pkt, j, fifo, j, read[j], j);
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
	free(write);
}





