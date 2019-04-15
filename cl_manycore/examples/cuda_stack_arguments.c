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

#include <bsg_manycore_driver.h>
#include <bsg_manycore_mem.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include "cuda_regression.h"


/*! 
 * Tests a kernel with > 8 arguments where the arguments spill over to the stack.
 * @param[in] argv this program takes one argument, which is the path to the binary it uses.
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/ Manycore binary in the bladerunner_v030_cuda branch of the BSG Manycore bitbucket repository. 
 * The kernel is called "kernel12". It sends a success packet if the arguments meet a condition and false otherwise.
 * Note - this test is not supported by Bladerunner version 0.3.0. 
 */

static int argv_base = 0x44444444; /* kernel expects the arguments to start at this value */
static int run_kernel (uint8_t fd, uint32_t eva_id, char *elf, tile_t tiles[], uint32_t num_tiles);

int main (int argc, char *argv[]) {
	static struct option options [] = {
		{ "help" , no_argument, 0, 'h' },
		{ /* sentinel */ }
	};

	static const char * opstring = "h";
	int ch;
	
	execname = argv[0];

	// process options
	while ((ch = getopt_long(argc, argv, opstring, options, NULL)) != -1) {
	switch (ch) {
		case 'h':
			usage();
			return HB_MC_SUCCESS;
		default:
			usage();
			return HB_MC_FAIL;
		}
	}

	argc -= optind;
	argv += optind;

	printf("Testing passing arguments on the stack .....\n");

	uint8_t fd; 
	hb_mc_init_host(&fd);
	
	tile_t tiles[1];
	uint32_t num_tiles = 4, num_tiles_x = 2, num_tiles_y = 2, origin_x = 0, origin_y = 1;
	create_tile_group(tiles, num_tiles_x, num_tiles_y, origin_x, origin_y); /* 1 x 1 tile group at (0, 1) */
	eva_id_t eva_id = 0;

	if (hb_mc_init_device(fd, eva_id, argv[0], &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return HB_MC_FAIL;
	}  

	int pass = run_kernel(fd, eva_id, argv[0], tiles, num_tiles);
	
	hb_mc_device_finish(fd, eva_id, tiles, num_tiles); /* freeze the tile and memory manager cleanup */
	
	return pass;
}
/*!
 * Runs a kernel that accepts 12 arguments. 
 * @param[in] fd the userspace file descriptor
 * @param[in] EVA to NPA mapping
 * @param[in] elf the path to the binary to use
 * @param[in] tiles the tiles of the tile group to run on
 * @param[in] num_tiles the number of tiles in the tile group
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure
 */
static int run_kernel (uint8_t fd, uint32_t eva_id, char *elf, tile_t tiles[], uint32_t num_tiles) {
	int argv[12];
	for (int i = 0; i < 12; i++) 
		argv[i] = argv_base + i;
	
	int error = hb_mc_device_launch(fd, eva_id, "kernel12", 12, argv, elf, tiles, num_tiles); /* launch the kernel */
	if (error != HB_MC_SUCCESS)
		return HB_MC_FAIL;
		
	hb_mc_packet_t kernel_result;
	hb_mc_read_fifo(fd, 1, &kernel_result); /* read back the result of the kernel */
	int pass = HB_MC_SUCCESS;
	if (hb_mc_request_packet_get_data(&kernel_result.request) == 0x1)
		printf("test passed.\n");
	else {
		pass = HB_MC_FAIL;
		printf("test failed.\n");
	}

	hb_mc_cuda_sync(fd, &tiles[0]); /* get the runtime signal packet */
	return pass;
}
