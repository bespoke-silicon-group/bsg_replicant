#ifndef CUDA_ADD_KERNEL_FOUR_TILES_2_2
#define CUDA_ADD_KERNEL_FOUR_TILES_2_2

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
 * Runs the addition kernel on a 2 x 2 tile group at (2, 2). 
 * @param[in] elf the path to the binary. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/ Manycore binary in the bladerunner_v030_cuda branch of the BSG Manycore bitbucket repository. 
 */
int cuda_add_kernel_four_tiles_2_2 (char *elf) {
	printf("Running the addition kernel on a 2 x 2 tile group at (2, 2).\n\n");
	
	uint8_t fd; 
	hb_mc_init_host(&fd);
	
	/* run on a 2 x 2 grid of tiles starting at (2, 2) */
	tile_t tiles[4];
	uint32_t num_tiles = 4, num_tiles_x = 2, num_tiles_y = 2, origin_x = 2, origin_y = 2;
	create_tile_group(tiles, num_tiles_x, num_tiles_y, origin_x, origin_y); /* 2 x 2 tile group at (2, 2) */
	eva_id_t eva_id = 0;
	
	if (hb_mc_init_device(fd, eva_id, elf, &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return HB_MC_FAIL;
	}  

	run_kernel_add(fd, eva_id, elf, tiles, num_tiles);
	
	hb_mc_device_finish(fd, eva_id, tiles, num_tiles); /* freeze the tile and memory manager cleanup */
	return HB_MC_SUCCESS; 
}

#endif
