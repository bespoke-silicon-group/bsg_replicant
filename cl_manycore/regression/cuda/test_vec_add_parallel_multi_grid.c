#include "test_vec_add_parallel_multi_grid.h"

/*!
 * Runs the addition kernel on two grid of 4 2x2 tile groups in parallel on a 4x4 grid. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_parallel_multi_grid/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore bitbucket repository.  
*/
int kernel_vec_add_parallel_multi_grid () {
	fprintf(stderr, "Running the CUDA Parallel Multi Grid Vector Addition Kernel on two grids of 4 2x2 tile groups.\n\n");

	srand(time);
	device_t device;
	uint8_t mesh_dim_x = 4;
	uint8_t mesh_dim_y = 4;
	uint8_t mesh_origin_x = 0;
	uint8_t mesh_origin_y = 1;
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/vec_add_parallel_multi_grid/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, mesh_dim_x, mesh_dim_y, mesh_origin_x, mesh_origin_y);

	uint32_t size_buffer = 256; 


	/* Prepare data for grid 1 */
	eva_t A_device_1, B_device_1, C_device_1; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device_1); /* allocate A1 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device_1); /* allocate B1 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device_1); /* allocate C1 on the device */

	uint32_t A_host_1[size_buffer]; /* allocate A1 on the host */ 
	uint32_t B_host_1[size_buffer]; /* allocate B1 on the host */
	for (int i = 0; i < size_buffer; i++) { /* fill A1 and B1 with arbitrary data */
		A_host_1[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host_1[i] = rand() % ((1 << 16) - 1); 
	}

	void *dst = (void *) ((intptr_t) A_device_1);
	void *src = (void *) &A_host_1[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A1 to the device  */	
	dst = (void *) ((intptr_t) B_device_1);
	src = (void *) &B_host_1[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B1 to the device */ 




	/* Prepare data for grid 2 */
	eva_t A_device_2, B_device_2, C_device_2; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device_2); /* allocate A2 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device_2); /* allocate B2 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device_2); /* allocate C2 on the device */

	uint32_t A_host_2[size_buffer]; /* allocate A2 on the host */ 
	uint32_t B_host_2[size_buffer]; /* allocate B2 on the host */
	for (int i = 0; i < size_buffer; i++) { /* fill A2 and B2 with arbitrary data */
		A_host_2[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host_2[i] = rand() % ((1 << 16) - 1); 
	}

	dst = (void *) ((intptr_t) A_device_2);
	src = (void *) &A_host_2[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A2 to the device  */	
	dst = (void *) ((intptr_t) B_device_2);
	src = (void *) &B_host_2[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B2 to the device */ 





	uint8_t grid_dim_x = 4;
	uint8_t grid_dim_y = 1;
	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	int argv_1[4] = {A_device_1, B_device_1, C_device_1, size_buffer};
	int argv_2[4] = {A_device_2, B_device_2, C_device_2, size_buffer};


	hb_mc_grid_init (&device, grid_dim_x, grid_dim_y, tg_dim_x, tg_dim_y, "kernel_vec_add", 4, argv_1);
	hb_mc_grid_init (&device, grid_dim_x, grid_dim_y, tg_dim_x, tg_dim_y, "kernel_vec_add", 4, argv_2);

	hb_mc_device_tile_groups_execute(&device);
	

	uint32_t C_host_1[size_buffer];
	src = (void *) ((intptr_t) C_device_1);
	dst = (void *) &C_host_1[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C1 to the host */


	uint32_t C_host_2[size_buffer];
	src = (void *) ((intptr_t) C_device_2);
	dst = (void *) &C_host_2[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C2 to the host */





	int mismatch = 0; 
	for (int i = 0; i < size_buffer; i++) {
		if (A_host_1[i] + B_host_1[i] == C_host_1[i]) {
			fprintf(stderr, "Success -- A1[%d] + B1[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host_1[i], B_host_1[i], C_host_1[i]);
		}
		else {
			fprintf(stderr, "Failed -- A1[%d] + B1[%d] =  0x%x + 0x%x != 0x%x\n", i, i , A_host_1[i], B_host_1[i], C_host_1[i]);
			mismatch = 1;
		}
	}	


	for (int i = 0; i < size_buffer; i++) {
		if (A_host_2[i] + B_host_2[i] == C_host_2[i]) {
			fprintf(stderr, "Success -- A2[%d] + B2[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host_2[i], B_host_2[i], C_host_2[i]);
		}
		else {
			fprintf(stderr, "Failed -- A2[%d] + B2[%d] =  0x%x + 0x%x != 0x%x\n", i, i , A_host_2[i], B_host_2[i], C_host_2[i]);
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
	bsg_pr_test_info("test_vec_add_parallel_multi_grid Regression Test (COSIMULATION)\n");
	int rc = kernel_vec_add_parallel_multi_grid();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_vec_add_parallel_multi_grid Regression Test (F1)\n");
	int rc = kernel_vec_add_parallel_multi_grid();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

