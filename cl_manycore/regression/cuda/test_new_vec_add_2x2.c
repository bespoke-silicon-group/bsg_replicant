#include "test_new_vec_add_2x2.h"

/*!
 * Runs the addition kernel on a 2x2 tile group at (0, 1). 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_2x2/ Manycore binary in the dev_cuda_tile_group_refactored branch of the BSG Manycore bitbucket repository.  
*/
int kernel_vec_add () {
	fprintf(stderr, "Running the CUDA Vector Addition Kernel on two 2x2 tile groups in parallel.\n\n");

	device_t device;
	uint8_t grid_dim_x = 4;
	uint8_t grid_dim_y = 4;
	uint8_t grid_origin_x = 0;
	uint8_t grid_origin_y = 1;
	eva_id_t eva_id = 0;
	char* ELF_PATH = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/new_vec_add_2x2/main.riscv";

	hb_mc_device_init(&device, eva_id, ELF_PATH, grid_dim_x, grid_dim_y, grid_origin_x, grid_origin_y);


	uint32_t size_buffer = 8; 
	eva_t A_device, B_device, C_device; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device); /* allocate A on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device); /* allocate B on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device); /* allocate C on the device */

	uint32_t A_host[size_buffer]; /* allocate A on the host */ 
	uint32_t B_host[size_buffer]; /* allocate B on the host */
	srand(0);
	for (int i = 0; i < size_buffer; i++) { /* fill A and B with arbitrary data */
		A_host[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host[i] = rand() % ((1 << 16) - 1); 
	}

	void *dst = (void *) ((intptr_t) A_device);
	void *src = (void *) &A_host[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A1 to the device  */	
	dst = (void *) ((intptr_t) B_device);
	src = (void *) &B_host[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B2 to the device */ 


	tile_group_t tg; 
	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	int argv[4] = {A_device, B_device, C_device, size_buffer / (tg_dim_x * tg_dim_y)};
	uint32_t finish_signal_addr = 0xC0DA;


	hb_mc_tile_group_init (&device, &tg, tg_dim_x, tg_dim_y, "kernel_vec_add", 4, argv, finish_signal_addr);

	hb_mc_device_launch(&device);
	
	hb_mc_tile_group_sync(&device, &(device.tile_groups[0]));

	hb_mc_tile_group_deallocate(&device, &tg); 
	




	uint32_t C_host[size_buffer];
	src = (void *) ((intptr_t) C_device);
	dst = (void *) &C_host[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C to the host */


	int mismatch = 0; 
	for (int i = 0; i < size_buffer; i++) {
		if (A_host[i] + B_host[i] == C_host[i]) {
			fprintf(stderr, "Success -- A[%d] + B[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host[i], B_host[i], C_host[i]);
		}
		else {
			fprintf(stderr, "Failed -- A[%d] + B[%d] =  0x%x + 0x%x != 0x%x\n", i, i , A_host[i], B_host[i], C_host[i]);
			mismatch = 1;
		}
	}	


	hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */
	

	if (mismatch)
		return HB_MC_FAIL;
	return HB_MC_SUCCESS;
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

