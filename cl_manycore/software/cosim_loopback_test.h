#ifndef COSIM_LOOPBACK_TEST_H
#define COSIM_LOOPBACK_TEST_H

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
#include "bsg_manycore_print.h"

void cosim_loopback_test () {
	
	printf("Running the Cosimulation Loopback test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd = 0; /* unused */

	uint8_t x = 0, y = 0;
	
	hb_mc_load_binary(fd, getenv("MAIN_LOOPBACK"), &x, &y, 1);

  	hb_mc_unfreeze(fd, 0, 0);

	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */	
	uint32_t *receive_packet = hb_mc_read_fifo(fd, 1, NULL);
	printf("Receive packet: ");
	hb_mc_print_hex((uint8_t *) receive_packet);
}

#endif
