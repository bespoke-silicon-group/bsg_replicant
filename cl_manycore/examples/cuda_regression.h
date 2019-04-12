#ifndef RUN_KERNEL_ADD_H
#define RUN_KERNEL_ADD_H


#include <getopt.h>
#include "bsg_manycore_driver.h"

/*! 
 * prints a Manycore packet's raw data.
 * @param[in] p the packet to print.
 * */
static void print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) {
		printf("%x ", (p[15-i] & 0xFF));
	}
	printf("\n");
}

/*!
 * creates a tile group with a specified origin
 * @param[out] tiles an array of tiles that will be set in row-order. This should be allocated by the caller
 * @param[out] the number of tiles in the tile group
 * @param[in] num_tiles_x the number of columns in the tile group
 * @param[in] num_tiles_y the number of rows in the tile group
 * @param[in] origin_x the x coordinate of the tile group's origin
 * @param[in] origin_y the y coordinate of the tile group's origin 
 * */
static void create_tile_group(tile_t tiles[], uint32_t num_tiles_x, uint32_t num_tiles_y, uint32_t origin_x, uint32_t origin_y) {
	/* create the tile group */
	for (int i = 0; i < num_tiles_y; i++) {
		for (int j = 0; j < num_tiles_x; j++) {
			int index = i * num_tiles_x + j;
			tiles[index].x = j + origin_x; 
			tiles[index].y = i + origin_y;
			tiles[index].origin_x = origin_x;
			tiles[index].origin_y = origin_y;
		}
	}
}


/*!
 * Runs the vector addition kernel on tiles that have been initialized with hb_mc_device_init(). 
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id EVA to NPA mapping
 * @param[in] elf path to the binary
 * @param[in] tiles tile group to run on
 * @param[in] num_tiles number of tiles in the tile group
 * */
static void run_kernel_add (uint8_t fd, uint32_t eva_id, char *elf, tile_t tiles[], uint32_t num_tiles) {
	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	printf("run_kernel_add(): start: 0x%x, size: 0x%x\n", start, size); /* if CUDA init is correct, start should be TODO and size should be TODO */
	
	uint32_t size_buffer = 16; 
	eva_t A_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t)); /* allocate A on the device */
	eva_t B_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t)); /* allocate B on the device */
	eva_t C_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t)); /* allocate C on the device */
	printf("run_kernel_add(): A's EVA 0x%x, B's EVA: 0x%x, C's EVA: 0x%x\n", A_device, B_device, C_device); /* if CUDA malloc is correct, A should be TODO, B should be TODO, C should be TODO */
 
	uint32_t A_host[size_buffer]; /* allocate A on the host */ 
	uint32_t B_host[size_buffer]; /* allocate B on the host */
	srand(0);
	for (int i = 0; i < size_buffer; i++) { /* fill A and B with arbitrary data */
		A_host[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host[i] = rand() % ((1 << 16) - 1); 
	}

	void *dst = (void *) A_device;
	void *src = (void *) &A_host[0];
	int error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A to the device  */	
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_add(): could not copy buffer A to device.\n");
	}
	dst = (void *) B_device;
	src = (void *) &B_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B to the device */ 
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_add(): could not copy buffer B to device.\n");
	}

	int argv[4] = {A_device, B_device, C_device, size_buffer / num_tiles};
	error = hb_mc_device_launch(fd, eva_id, "kernel_add", 4, argv, elf, tiles, num_tiles); /* launch the kernel */

	hb_mc_cuda_sync(fd, &tiles[0]); /* if CUDA sync is correct, this program won't hang here. */
	
	uint32_t C_host[size_buffer];
	src = (void *) C_device;
	dst = (void *) &C_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host */
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_add(): Unable to copy A from device.\n");
	}
	
	printf("Finished vector addition: \n");
	for (int i = 0; i < size_buffer; i++) {
		printf("A[%d] + B[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host[i], B_host[i], C_host[i]);
	}	

	hb_mc_device_free(eva_id, A_device); /* free A on device */
	hb_mc_device_free(eva_id, B_device); /* free B on device */
	hb_mc_device_free(eva_id, C_device); /* free C on device */
}

static const char *execname = "";

#define pr_usage(fmt, ...)			\
    fprintf(stderr, fmt, ##__VA_ARGS__)

static void base_usage(const char *name, const char *help)
{
    pr_usage("\t%s\t%s\n", name, help);
}

static void argument_usage(const char *argname, const char *arghelp)
{
    base_usage(argname, arghelp);
}

static void option_usage(const char *opname, const char *ophelp)
{
    base_usage(opname, ophelp);
}

static void usage() {
    pr_usage("usage: %s [OPTIONS] ARG1\n", execname);

    pr_usage("arguments:\n");
    argument_usage("ARG1", "Path to the Manycore-Hammerblade binary.");

    pr_usage("options:\n");
    option_usage("-h,--help", "Print this help message");    
}
#endif
