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
	hb_mc_request_packet_t manycore_finish;
	hb_mc_read_fifo(fd, 1, (hb_mc_packet_t *) &manycore_finish);	

	uint32_t addr = hb_mc_request_packet_get_addr(&manycore_finish);
	uint32_t data = hb_mc_request_packet_get_data(&manycore_finish);
	uint32_t op_ex = hb_mc_request_packet_get_op_ex(&manycore_finish);
	uint32_t x_src = hb_mc_request_packet_get_x_src(&manycore_finish);
	uint32_t y_src = hb_mc_request_packet_get_y_src(&manycore_finish);
	uint32_t x_dst = hb_mc_request_packet_get_x_dst(&manycore_finish);
	uint32_t y_dst = hb_mc_request_packet_get_y_dst(&manycore_finish);
	uint32_t op = hb_mc_request_packet_get_op(&manycore_finish);
	printf("Manycore finish packet received at Address 0x%x at coordinates (0x%x, 0x%x) from (0x%x, 0x%x). Operation: 0x%x, Data: 0x%x\n", addr, x_dst, y_dst, x_src, y_src, op, data & op_ex);
 }

#endif
