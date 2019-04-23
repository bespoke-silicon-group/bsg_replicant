#include "test_bsg_cuda_lite_runtime_empty.h"

/*!
 * Runs the empty kernel on tiles that have been initialized with hb_mc_device_init(). 
 * The purpose of this test is to test read/write to DRAM
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id EVA to NPA mapping
 * @param[in] elf path to the binary
 * @param[in] tiles tile group to run on
 * @param[in] num_tiles number of tiles in the tile group
 * */
static int run_kernel_empty (uint8_t fd, uint32_t eva_id, char *elf, tile_t tiles[], uint32_t num_tiles) {
	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	printf("run_kernel_empty(): start: 0x%x, size: 0x%x\n", start, size); /* if CUDA init is correct, start should be TODO and size should be TODO */
	
	uint32_t size_buffer = 1024; 
	eva_t A_device, B_device; 
	if(hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t), &A_device) != HB_MC_SUCCESS) { /* allocate A on the device */
		printf("Failed to allocate A on device\n");
		return;
	}
	if(hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t), &B_device) != HB_MC_SUCCESS) { /* allocate B on the device */
		printf("Failed to allocate B on device\n");
		return;
	}
	printf("run_kernel_empty(): A's EVA 0x%x, B's EVA: 0x%x\n", A_device, B_device); /* if CUDA malloc is correct, A should be TODO, B should be TODO */

	/* zero out the vectors */
/*
	uint32_t zeros[16] = {};
	
	if (hb_mc_device_memcpy (fd, eva_id, (void *) A_device, &zeros[0], size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device) != HB_MC_SUCCESS) { 
		printf("Could not zero out A.\n");
		return;
	}
	if (hb_mc_device_memcpy (fd, eva_id, (void *) B_device, &zeros[0], size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device) != HB_MC_SUCCESS) { 
		printf("Could not zero out B.\n");
		return;
	}
	if (hb_mc_device_memcpy (fd, eva_id, (void *) C_device, &zeros[0], size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device) != HB_MC_SUCCESS) { 
		printf("Could not zero out C.\n");
		return;
	}
*/

 
	uint32_t A_host[size_buffer]; /* allocate A on the host */ 
	uint32_t B_host[size_buffer]; /* allocate B on the host */
	srand(0);
	for (int i = 0; i < size_buffer; i++) { /* fill A and B with arbitrary data */
		A_host[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host[i] = rand() % ((1 << 16) - 1); 
	}

	void *dst = (void *) ((intptr_t) A_device);
	void *src = (void *) &A_host[0];
	int error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A to the device  */	
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_empty(): could not copy buffer A to device.\n");
	}
	dst = (void *) ((intptr_t) B_device);
	src = (void *) &B_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B to the device */ 
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_empty(): could not copy buffer B to device.\n");
	}

	int argv[2] = {A_device, B_device};
	error = hb_mc_device_launch(fd, eva_id, "kernel_empty", 2, argv, getenv("ELF_CUDA_EMPTY"), tiles, num_tiles); /* launch the kernel */

	hb_mc_cuda_sync(fd, &tiles[0]); /* if CUDA sync is correct, this program won't hang here. */
	uint32_t A_host_read_back[size_buffer];
	src = (void *) ((intptr_t) A_device);
	dst = (void *) &A_host_read_back[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host */
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_add(): Unable to copy A back from device.\n");
	}

	uint32_t B_host_read_back[size_buffer];
	src = (void *) ((intptr_t) B_device);
	dst = (void *) &B_host_read_back[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host */
	if (error != HB_MC_SUCCESS) {
		printf("run_kernel_add(): Unable to copy B back from device.\n");
	}

	int mismatch = 0; 
	for (int i = 0; i < size_buffer; i++){
		if (A_host[i] == A_host_read_back[i])	
			printf ("Match -- A_host[%d] = %d, A_host_read_back[%d] =%d.\n", i, A_host[i], i, A_host_read_back[i]);
		else {
			printf ("Mismatch -- A_host[%d] = %d, A_host_read_back[%d] =%d.\n", i, A_host[i], i, A_host_read_back[i]);
			mismatch = 1;
		}
		if (B_host[i] == B_host_read_back[i])	
			printf ("Match -- B_host[%d] = %d, B_host_read_back[%d] =%d.\n", i, B_host[i], i, B_host_read_back[i]);
		else {
			printf ("Mismatch -- B_host[%d] = %d, B_host_read_back[%d] =%d.\n", i, B_host[i], i, B_host_read_back[i]);
			mismatch = 1;
		}
	}
	hb_mc_device_free(eva_id, A_device); /* free A on device */
	hb_mc_device_free(eva_id, B_device); /* free B on device */

	if (mismatch)
		return HB_MC_FAIL;
	return HB_MC_SUCCESS;
}

/*!
 * Runs the empty kernel on a 2 x 2 tile group at (0, 1). 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime_empty/ Manycore binary in the bladerunner_v030_cuda branch of the BSG Manycore bitbucket repository. This test assumes there is an environment variable `ELF_CUDA_EMPTY` that points to this binary. 
*/
int test_empty_kernel () {
	printf("Running the DRAM read write test on a tile group of size 2x2.\n\n");

	uint8_t fd; 
	hb_mc_init_host(&fd);
	/* run on a 2 x 2 grid of tiles starting at (0, 1) */
	tile_t tiles[4];
	uint32_t num_tiles = 4, num_tiles_x = 2, num_tiles_y = 2, origin_x = 0, origin_y = 1;
	create_tile_group(tiles, num_tiles_x, num_tiles_y, origin_x, origin_y); /* 2 x 2 tile group at (0, 1) */
	eva_id_t eva_id = 0;
	
	if (hb_mc_init_device(fd, eva_id, getenv("ELF_CUDA_EMPTY"), &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return HB_MC_FAIL;
	}  

	int error = run_kernel_empty(fd, eva_id, getenv("ELF_CUDA_EMPTY"), tiles, num_tiles);
	
	hb_mc_device_finish(fd, eva_id, tiles, num_tiles); /* freeze the tile and memory manager cleanup */
	return error;	
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_bsg_cuda_lite_runtime_empty Regression Test (COSIMULATION)\n");
	int rc = test_empty_kernel();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_bsg_cuda_lite_runtime_empty Regression Test (COSIMULATION)\n");
	int rc = test_empty_kernel();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

