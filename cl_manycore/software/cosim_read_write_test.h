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
#include "bsg_manycore_packet.h"

  void print_hex (uint8_t *p) {
  	for (int i = 0; i < 16; i++) {
  		printf("%x ", (p[15-i] & 0xFF));
  	}
  	printf("\n");
  }




void cosim_read_write_test () {








	printf("Running the Cosimulation Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd; 
	hb_mc_init_host(&fd);	

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
	hb_mc_response_packet_t buf[1];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
	
	if (read == HB_MC_SUCCESS) {
		printf("read packet: ");
		print_hex((uint8_t *) &buf[0]);
	}
	
	else {
		printf("read from tile failed.\n");
	}

	return;
}

#endif
