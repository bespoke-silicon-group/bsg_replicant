#ifndef COSIM_DEVICE_INIT_TEST_iH
#define COSIM_DEVICE_INIT_TEST_H

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

void print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) {
		printf("%x ", (p[15-i] & 0xFF));
	}
	printf("\n");
}


void cosim_device_init_test () {
	
	printf("Cosimulation test of hb_mc_init_device().\n\n");
	uint8_t fd; 
	hb_mc_init_host(&fd);
	
	/* run on tile (0, 1) */
	tile_t tiles[1];
	tiles[0].x = 0;
	tiles[0].y = 1;
	tiles[0].origin_x = 0;
	tiles[0].origin_y = 1;
	eva_id_t eva_id = 0;
	uint32_t num_tiles = 1;
	if (hb_mc_init_device(fd, eva_id, getenv("MAIN_LOOPBACK"), &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return;
	}  

	/* make sure memory manager was initialized properly */
	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	printf("start: 0x%x, size: 0x%x\n", start, size);
	hb_mc_device_finish(fd, eva_id, tiles, num_tiles);
	return;
}

#endif
