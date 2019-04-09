#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>

#include <bsg_manycore_driver.h>
#include "bsg_manycore_mem.h"
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>

/**
 *  This tests uses the software/spmd/bsg_cuda_lite_runtime/ Manycore binary. Compile the program and save its path in an environment variable called "elf_cuda_add" before running this test. The enviornment variable should include the binary name "main.riscv."
 *
 */
#define pr_usage(fmt, ...)			\
    fprintf(stderr, fmt, ##__VA_ARGS__)

static const char *execname = "";

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
    argument_usage("ARG1", "Path to Manycore Vector Addition binary.");

    pr_usage("options:\n");
    option_usage("-h,--help", "Print this help message");    
}

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
			exit(0);
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

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
	printf("%s\n",*argv);
	if (hb_mc_init_device(fd, eva_id, argv[0], &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return 0;
	}  

	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	printf("start: 0x%x, size: 0x%x\n", start, size); /* if CUDA init is correct, start should be TODO and size should be TODO */
	
	uint32_t size_buffer = 16; 
	eva_t A_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t)); /* allocate A on the device */
	eva_t B_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t)); /* allocate B on the device */
	eva_t C_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t)); /* allocate C on the device */
	printf("A's EVA 0x%x, B's EVA: 0x%x, C's EVA: 0x%x\n", A_device, B_device, C_device); /* if CUDA malloc is correct, A should be TODO, B should be TODO, C should be TODO */
 

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
		printf("cosim_cuda_test(): could not copy buffer A to device.\n");
	}
	dst = (void *) B_device;
	src = (void *) &B_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B to the device */ 
	if (error != HB_MC_SUCCESS) {
		printf("cosim_cuda_test(): could not copy buffer B to device.\n");
	}
	int argv_kernel[4] = {A_device, B_device, C_device, size_buffer};
	error = hb_mc_device_launch(fd, eva_id, "kernel_add", 4, argv_kernel, argv[0], &tiles[0]); /* launch the kernel */
	hb_mc_cuda_sync (fd, &tiles[0]); /* if CUDA sync is correct, this program won't hang here. */


	hb_mc_response_packet_t C_host[size_buffer];
	src = (void *) C_device;
	dst = (void *) &C_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host */
	if (error != HB_MC_SUCCESS) {
		printf("cosim_cuda_test(): Unable to copy A from device.\n");
	}
	
	printf("Finished vector addition: \n");
	for (int i = 0; i < size_buffer; i++) {
		printf("A[%d] + B[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host[i], B_host[i], hb_mc_response_packet_get_data(&C_host[i])); 
	}	

	hb_mc_device_free(eva_id, A_device); /* free A on device */
	hb_mc_device_free(eva_id, B_device); /* free B on device */
	hb_mc_device_free(eva_id, C_device); /* free C on device */


	hb_mc_device_finish(fd, eva_id, tiles, num_tiles); /* freeze the tile and memory manager cleanup */

	return 0;
}
