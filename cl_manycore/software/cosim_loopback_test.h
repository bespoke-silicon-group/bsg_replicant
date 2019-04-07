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
#include "bsg_manycore_packet.h"

void cosim_loopback_test () {

  void print_hex (uint8_t *p) {
  	for (int i = 0; i < 16; i++) {
  		printf("%x ", (p[15-i] & 0xFF));
  	}
  	printf("\n");
  }

	printf("Running the Cosimulation Loopback test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd;
	hb_mc_init_host(&fd);

	uint8_t x = 0, y = 1;

	hb_mc_freeze(fd, 0, 1);
	
	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);
	
	hb_mc_init_cache_tag(fd, 0, 5);
	hb_mc_init_cache_tag(fd, 1, 5);
	hb_mc_init_cache_tag(fd, 2, 5);
	hb_mc_init_cache_tag(fd, 3, 5);

	hb_mc_freeze(fd, 0, 1);
	
	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);

	hb_mc_load_binary(fd, getenv("MAIN_LOOPBACK"), &x, &y, 1);

  	hb_mc_unfreeze(fd, 0, 1);

	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */
	hb_mc_request_packet_t manycore_finish = {3, 0, 0, 1, 0, 0xF, 0x1, 0x3ab4, {0, 0}};
	hb_mc_device_sync (fd, &manycore_finish);
	printf("Manycore finish packet received.\n");
 }

#endif
