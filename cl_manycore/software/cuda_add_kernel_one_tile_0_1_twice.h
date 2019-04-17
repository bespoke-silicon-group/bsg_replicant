#ifndef CUDA_ADD_KERNEL_ONE_TILE_0_1_TWICE
#define CUDA_ADD_KERNEL_ONE_TILE_0_1_TWICE

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

#include "cuda_regression.h"

/*!
 * Runs the addition kernel on 1 tile at (0, 1) twice. 
 * @param[in] elf the path to the binary. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/ Manycore binary in the bladerunner_v030_cuda branch of the BSG Manycore bitbucket repository. 
 */
int cuda_add_kernel_one_tile_0_1_twice (char *elf) {
	printf("Running the addition kernel on a 1 x 1 tile group at (0, 1) twice.\n\n");
	uint8_t fd; 
	hb_mc_init_host(&fd);
	/* run on tile (0, 1) */
	tile_t tiles[1];
	uint32_t num_tiles = 1, num_tiles_x = 1, num_tiles_y = 1, origin_x = 0, origin_y = 1;
	create_tile_group(tiles, num_tiles_x, num_tiles_y, origin_x, origin_y); /* 1 x 1 tile group at (0, 1) */
	eva_id_t eva_id = 0;
	if (hb_mc_init_device(fd, eva_id, elf, &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return HB_MC_FAIL;
	}  

	printf("The first time ....\n");
	run_kernel_add(fd, eva_id, elf, tiles, num_tiles);
	printf("The second time ....\n");
	run_kernel_add(fd, eva_id, elf, tiles, num_tiles);

	hb_mc_device_finish(fd, eva_id, tiles, num_tiles); /* freeze the tile and memory manager cleanup */
	return HB_MC_SUCCESS;
}

#endif
