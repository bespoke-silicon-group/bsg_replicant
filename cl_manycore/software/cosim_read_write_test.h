#ifndef COSIM_READ_WRITE_TEST_H
#define COSIM_READ_WRITE_TEST_H

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


void cosim_read_write_test () {

  void print_hex (uint8_t *p) {
  	for (int i = 0; i < 16; i++) {
  		printf("%x ", (p[15-i] & 0xFF));
  	}
  	printf("\n");
  }

	printf("Running the Cosimulation Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd = 0; /* unused */
	/* store data in tile */
	uint32_t data = 0xABCD;
	printf("write to DMEM\n");
	int write = hb_mc_copy_to_epa(0, 0, 1, DMEM_BASE >> 2, &data, 1);

	if (write != HB_MC_SUCCESS) {
		printf("writing data to tile (0, 0)'s DMEM failed.\n");
		return;
	}
	printf("write success\n");
	/* read back data */
	uint32_t **buf = (uint32_t **) calloc(1, sizeof(uint32_t *));
	int read = hb_mc_copy_from_epa(fd, buf, 0, 1, DMEM_BASE >> 2, 1); 
	printf("completed read.\n");
	if (read == HB_MC_SUCCESS) {
		printf("read packet: ");
		print_hex((uint8_t *) buf[0]);
	}
	
	else {
		printf("read from tile failed.\n");
	}

	return;
}

#endif
