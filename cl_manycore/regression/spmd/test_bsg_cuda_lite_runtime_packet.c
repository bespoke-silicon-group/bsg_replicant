#include "test_bsg_cuda_lite_runtime_packet.h"
/*
 * Performs and reduction on elements of an array and returns the result.
 * */

static int array_reduce (uint8_t *array, uint32_t size){
	int res = 1; 
	for (int i = 0; i < size; i ++)
		res &= array[i];
	return res;
}
/*!
 * Runs the packet kernel on tiles that have been initialized with hb_mc_device_init(). 
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id EVA to NPA mapping
 * @param[in] elf path to the binary
 * @param[in] tiles tile group to run on
 * @param[in] num_tiles number of tiles in the tile group
 * */
static void run_kernel_packet (uint8_t fd, uint32_t eva_id, char *elf, tile_t tiles[], uint32_t num_tiles) {
	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	printf("run_kernel_packet(): start: 0x%x, size: 0x%x\n", start, size); /* if CUDA init is correct, start should be TODO and size should be TODO */
	

	int argv = 0;
	int error = hb_mc_device_launch(fd, eva_id, "kernel_packet", 0, argv, elf, tiles, num_tiles); /* launch the kernel */

	
	uint8_t packets[16] = {}; /* 16 is the maximum number of tiles */
	for (int i = num_tiles; i < 16; i++) /* the bit vector for the tiles not in tile group is set to 1 */
		packets[i] = 1;
	hb_mc_request_packet_t recv;
	uint8_t host_x = hb_mc_get_num_x()-1;
	uint8_t host_y = 0; 

	fprintf(stderr, "Waiting for packets ... \n");
	while( array_reduce(packets, num_tiles) != 1){
		hb_mc_read_fifo(fd, 1, (hb_mc_packet_t *) &recv); // Wait for manycore to send a packet 
		printf ("Received Packet: src (%d,%d), dst (%d,%d), data: 0x%x, addr: 0x%x.\n", recv.x_src, recv.y_src, recv.x_dst, recv.y_dst, recv.data, recv.addr); 
		if (recv.x_dst == host_x && recv.y_dst == host_y && recv.data < num_tiles && recv.addr == 0x2000) /* arbitrary address used in bsg_cuda_lite_runtime_packet/main.riscv */
			packets[recv.data] = 1;
	}
	fprintf(stderr, "Waiting for finish packet...\n");

	hb_mc_cuda_sync(fd, &tiles[0]); /* if CUDA sync is correct, this program won't hang here. */

	printf("Finished packet kernel: \n");
}

/*!
 * Runs the packet kernel on a 2 x 2 tile group at (0, 1). 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/ Manycore binary in the bladerunner_v030_cuda branch of the BSG Manycore bitbucket repository. This test assumes there is an environment variable `ELF_CUDA_PACKET` that points to this binary. 
*/
int test_packet_kernel () {
	printf("Running the CUDA Packet Kernel on a tile group of size 1x1.\n\n");

	uint8_t fd; 
	hb_mc_host_init(&fd);
	fprintf(stderr, "ran hb_mc_host_init().\n");
	/* run on a 2 x 2 grid of tiles starting at (0, 1) */
	tile_t tiles[4];
	uint32_t num_tiles = 4, num_tiles_x = 2, num_tiles_y = 2, origin_x = 0, origin_y = 1;
	create_tile_group(tiles, num_tiles_x, num_tiles_y, origin_x, origin_y); /* 2 x 2 tile group at (0, 1) */
	fprintf(stderr, "ran create_tile_group().\n");
	eva_id_t eva_id = 0;
	
	char* ELF_CUDA_PACKET = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/" "bsg_cuda_lite_runtime_packet/main.riscv";

	if (hb_mc_init_device(fd, eva_id, ELF_CUDA_PACKET, &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return HB_MC_FAIL;
	}  
	fprintf(stderr, "ran hb_mc_init_device().\n");
	run_kernel_packet(fd, eva_id, ELF_CUDA_PACKET, tiles, num_tiles);
	fprintf(stderr, "ran run_kernel_packet().\n");
	hb_mc_device_finish(fd, eva_id, tiles, num_tiles); /* freeze the tile and memory manager cleanup */
	return HB_MC_SUCCESS; 
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_bsg_cuda_lite_runtime kernel_packet Regression Test (COSIMULATION)\n");
	int rc = test_packet_kernel();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_bsg_cuda_lite_runtime kernel_packet Regression Test (COSIMULATION)\n");
	int rc = test_packet_kernel();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

