#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

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

	loop_test(100, test_fifos);

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
	write = calloc(num_fsb * DW_PER_FSB, sizeof(int)); 	

	for (int i = 0; i < 10; i++) {
		/* clear interrupts */
		clear_int(i);

		/* write to fifo i */
		for (int fsb_p = 0; fsb_p < num_fsb; fsb_p++) {
			for(int j = 0; j < 4; j++) { /* make write packet */
				write[fsb_p*DW_PER_FSB + j] = j;
			}
			deploy_write_fifo(i, &write[fsb_p*4]); /* write the packets */
		}

		usleep(100);
		
		/* read from fifo i */
		bool pass = true;
		for (int fsb_p = 0; fsb_p < num_fsb; fsb_p++) {
			read = deploy_read_fifo(i);
			if (!read) {
				printf("fifo test %d failed. No read data for fsb packet %d received.\n", i, fsb_p);
				pass = false;
			}
			else {
				for (int j = 0; j < 2; j++) {
					if (read[j] != j) {
						printf("fifo test %d failed. Mismatch at fsb packet %d, DW %d: Received %d instead of %x. \n", i, fsb_p, j, read[j], j);
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
			printf("fifo test %d passed.\n", i) ;
		}
	}
}





