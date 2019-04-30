#include "test_vec_add_2x2.h"

/*!
 * Runs the vector addition kernel on tiles that have been initialized with hb_mc_device_init(). 
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id EVA to NPA mapping
 * @param[in] elf path to the binary
 * @param[in] tiles tile group to run on
 * @param[in] num_tiles number of tiles in the tile group
 * */
static int run_kernel_vec_add (uint8_t fd, uint32_t eva_id, char *elf, tile_t tiles[], uint32_t num_tiles) {
	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	fprintf(stderr, "run_kernel_vec_add(): start: 0x%x, size: 0x%x\n", start, size); /* if CUDA init is correct, start should be TODO and size should be TODO */

	uint32_t size_buffer = 16; 
	eva_t A_device, B_device, C_device; 
	if(hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t), &A_device) != HB_MC_SUCCESS) { /* allocate A on the device */
		fprintf(stderr, "hb_mc_device_malloc(): failed to allocate A on device\n");
		return HB_MC_FAIL;
	}
	if(hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t), &B_device) != HB_MC_SUCCESS) { /* allocate B on the device */
		fprintf(stderr, "hb_mc_device_malloc(): failed to allocate B on device\n");
		return HB_MC_FAIL;
	}
	if(hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t), &C_device) != HB_MC_SUCCESS) { /* allocate C on the device */
		fprintf(stderr, "hb_mcdevice_malloc(): failed to allocate C on device\n");
		return HB_MC_FAIL;
	}
	fprintf(stderr, "run_kernel_vec_add(): A's EVA 0x%x, B's EVA: 0x%x, C's EVA: 0x%x\n", A_device, B_device, C_device); /* if CUDA malloc is correct, A should be TODO, B should be TODO, C should be TODO */

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
		fprintf(stderr, "hb_mc_device_memcpy(): failed to copy buffer A to device.\n");
		return HB_MC_FAIL;
	}

	dst = (void *) ((intptr_t) B_device);
	src = (void *) &B_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B to the device */ 
	if (error != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_device_memcpy(): failed to copy buffer B to device.\n");
	}

	int argv[4] = {A_device, B_device, C_device, size_buffer / num_tiles};
	error = hb_mc_device_launch(fd, eva_id, "kernel_vec_add", 4, argv, elf, tiles, num_tiles); /* launch the kernel */
	if (error != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_device_launch(): failed to launch device.\n"); 
		return HB_MC_FAIL;
	}

	hb_mc_cuda_sync(fd, &tiles[0]); /* if CUDA sync is correct, this program won't hang here. */

	uint32_t C_host[size_buffer];
	src = (void *) ((intptr_t) C_device);
	dst = (void *) &C_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host */
	if (error != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_device_memcpy(): failed copy C from device.\n");
		return HB_MC_FAIL;
	}
	
	fprintf(stderr, "Finished vector addition: \n");

	int mismatch = 0; 
	for (int i = 0; i < size_buffer; i++) {
		if (A_host[i] + B_host[i] == C_host[i]) {
			fprintf(stderr, "Success -- A[%d] + B[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host[i], B_host[i], C_host[i]);
		}
		else {
			fprintf(stderr, "Fail -- A[%d] + B[%d] =  0x%x + 0x%x != 0x%x\n", i, i , A_host[i], B_host[i], C_host[i]);
			mismatch = 1;
		}
	}	

	hb_mc_device_free(eva_id, A_device); /* free A on device */
	hb_mc_device_free(eva_id, B_device); /* free B on device */
	hb_mc_device_free(eva_id, C_device); /* free C on device */

	if (mismatch)
		return HB_MC_FAIL;
	return HB_MC_SUCCESS;
}

/*!
 * Runs the addition kernel on a 2x2 tile group at (0, 1). 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_2x2/ Manycore binary in the dev_cuda_tile_group_refactored branch of the BSG Manycore bitbucket repository.  
*/
int kernel_vec_add () {
	fprintf(stderr, "Running the CUDA Addition Kernel on a tile group of size 2x2.\n\n");

//	uint8_t fd; 
	device_t device;
//	tile_t tiles[4];
//	uint32_t num_tiles = 4, num_tiles_x = 2, num_tiles_y = 2, origin_x = 0, origin_y = 1;
//	create_tile_group(tiles, num_tiles_x, num_tiles_y, origin_x, origin_y); /* 2x2 tile group at (0, 1) */
	uint8_t dim_x = 2, dim_y = 2, origin_x = 0, origin_y = 1;
	eva_id_t eva_id = 0;

	char* ELF_CUDA_ADD = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/vec_add_2x2/main.riscv";

	if (hb_mc_device_init(&device, eva_id, ELF_CUDA_ADD, dim_x, dim_y, origin_x, origin_y) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_device_init(): failed to  initialize device.\n");
		return HB_MC_FAIL;
	}  

	int error = run_kernel_vec_add(device.fd, eva_id, ELF_CUDA_ADD, device.grid->tiles, 4);
	
	hb_mc_device_finish(device.fd, eva_id, device.grid->tiles, 4); /* freeze the tile and memory manager cleanup */
	return error; 
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_bsg_cuda_lite_runtime_vec_add Regression Test (COSIMULATION)\n");
	int rc = kernel_vec_add();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_bsg_cuda_lite_runtime_vec_add Regression Test (F1)\n");
	int rc = kernel_vec_add();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

