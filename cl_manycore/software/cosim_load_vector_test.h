#ifndef COSIM_LOAD_VECTOR_TEST_H
#define COSIM_LOAD_VECTOR_TEST_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "bsg_manycore_driver.h"
#include "bsg_manycore_mem.h"
#include "bsg_manycore_loader.h"
#include "bsg_manycore_errno.h"
#include "bsg_manycore_packet.h"


void cosim_load_vector_test () {

  void print_hex (uint8_t *p) {
  	for (int i = 0; i < 16; i++) {
  		printf("%x ", (p[15-i] & 0xFF));
  	}
  	printf("\n");
  }

	printf("Running the Cosimulation Load Vector test on the Manycore with 4 x 4 dimensions.\n\n");
	
	
	uint8_t fd = 0; 
	hb_mc_init_host(&fd);	

	uint32_t n = 10; 
	uint32_t data[n];
	srand(0);
	for (int i = 0; i < n; i++) {
		data[i] = rand();
		printf("random number %d: 0x%x\n", i, data[i]);
	}

	/* store data in tile */
	int write = hb_mc_copy_to_epa(fd, 0, 1, DMEM_BASE >> 2, (uint32_t *) &data[0], n);

	if (write != HB_MC_SUCCESS) {
		printf("writing data to tile (0, 0)'s DMEM failed.\n");
		return;
	}

	/* read back data */
	response_packet_t buf[n];
	int read = hb_mc_copy_from_epa(fd, (void *) &buf[0], 0, 1, DMEM_BASE >> 2, n); 
	
	if (read == HB_MC_SUCCESS) {
		printf("read packets: \n");
		for (int i = 0; i < n; i++) {
			print_hex((uint8_t *) &buf[i]);
		}
	}
	
	else {
		printf("read from tile failed.\n");
	}

	return;
}

#endif
