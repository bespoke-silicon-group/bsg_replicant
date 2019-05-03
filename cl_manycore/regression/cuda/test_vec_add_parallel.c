#include "test_vec_add_parallel.h"

/*!
 * Runs the addition kernel on three 2x2 tile groups in parallel. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_parallel/ Manycore binary in the dev_cuda_tile_group_refactored branch of the BSG Manycore bitbucket repository.  
*/
int kernel_vec_add_parallel () {
	fprintf(stderr, "Running the CUDA Vector Addition Kernel on three 2x2 tile groups in parallel.\n\n");

	device_t device;
	uint8_t grid_dim_x = 4;
	uint8_t grid_dim_y = 4;
	uint8_t grid_origin_x = 0;
	uint8_t grid_origin_y = 1;
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/vec_add_parallel/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, grid_dim_x, grid_dim_y, grid_origin_x, grid_origin_y);


	uint32_t size_buffer = 4; 

	eva_t A_device_1, B_device_1, C_device_1; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device_1); /* allocate A1 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device_1); /* allocate B1 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device_1); /* allocate C1 on the device */

	uint32_t A_host_1[size_buffer]; /* allocate A1 on the host */ 
	uint32_t B_host_1[size_buffer]; /* allocate B1 on the host */
	srand(0);
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


	eva_t A_device_2, B_device_2, C_device_2; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device_2); /* allocate A2 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device_2); /* allocate B2 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device_2); /* allocate C2 on the device */

	uint32_t A_host_2[size_buffer]; /* allocate A2 on the host */ 
	uint32_t B_host_2[size_buffer]; /* allocate B2 on the host */
	srand(0);
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


	eva_t A_device_3, B_device_3, C_device_3; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device_3); /* allocate A3 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device_3); /* allocate B3 on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device_3); /* allocate C3 on the device */

	uint32_t A_host_3[size_buffer]; /* allocate A3 on the host */ 
	uint32_t B_host_3[size_buffer]; /* allocate B3 on the host */
	srand(0);
	for (int i = 0; i < size_buffer; i++) { /* fill A3 and B3 with arbitrary data */
		A_host_3[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host_3[i] = rand() % ((1 << 16) - 1); 
	}

	dst = (void *) ((intptr_t) A_device_3);
	src = (void *) &A_host_3[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A3 to the device  */	
	dst = (void *) ((intptr_t) B_device_3);
	src = (void *) &B_host_3[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B3 to the device */ 



	tile_group_t tg_1, tg_2, tg_3; 
	uint8_t tg_dim_x_1 = 2, tg_dim_x_2 = 2, tg_dim_x_3 = 2;
	uint8_t tg_dim_y_1 = 2, tg_dim_y_2 = 2, tg_dim_y_3 = 2;

	int argv_1[4] = {A_device_1, B_device_1, C_device_1, size_buffer / (tg_dim_x_1 * tg_dim_y_1)};
	int argv_2[4] = {B_device_2, B_device_2, C_device_2, size_buffer / (tg_dim_x_2 * tg_dim_y_2)};
	int argv_3[4] = {C_device_3, B_device_3, C_device_3, size_buffer / (tg_dim_x_3 * tg_dim_y_3)};

	uint32_t finish_signal_addr_1 = 0xC0D0;
	uint32_t finish_signal_addr_2 = 0xC0D4;
	uint32_t finish_signal_addr_3 = 0xC0D8;

	hb_mc_tile_group_init (&device, &tg_1, tg_dim_x_1, tg_dim_y_1, "kernel_vec_add", 4, argv_1, finish_signal_addr_1);
	hb_mc_tile_group_init (&device, &tg_2, tg_dim_x_2, tg_dim_y_2, "kernel_vec_add", 4, argv_2, finish_signal_addr_2);
	hb_mc_tile_group_init (&device, &tg_3, tg_dim_x_3, tg_dim_y_3, "kernel_vec_add", 4, argv_3, finish_signal_addr_3);

	fprintf(stderr, "INIT.\n");


	fprintf(stderr, "IDs: %d, %d, %d.\n", device.tile_groups[0].id, device.tile_groups[1].id, device.tile_groups[2].id);



	hb_mc_device_launch(&device);
	
	fprintf(stderr, "LAUNCHED.\n");

	uint32_t C_host_1[size_buffer];
	src = (void *) ((intptr_t) C_device_1);
	dst = (void *) &C_host_1[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C1 to the host */

	uint32_t C_host_2[size_buffer];
	src = (void *) ((intptr_t) C_device_2);
	dst = (void *) &C_host_2[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C2 to the host */

	uint32_t C_host_3[size_buffer];
	src = (void *) ((intptr_t) C_device_3);
	dst = (void *) &C_host_3[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C3 to the host */

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

	for (int i = 0; i < size_buffer; i++) {
		if (A_host_3[i] + B_host_3[i] == C_host_3[i]) {
			fprintf(stderr, "Success -- A3[%d] + B3[%d] =  0x%x + 0x%x = 0x%x\n", i, i , A_host_3[i], B_host_3[i], C_host_3[i]);
		}
		else {
			fprintf(stderr, "Failed -- A3[%d] + B3[%d] =  0x%x + 0x%x != 0x%x\n", i, i , A_host_3[i], B_host_3[i], C_host_3[i]);
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
	bsg_pr_test_info("test_vec_add_parallel Regression Test (COSIMULATION)\n");
	int rc = kernel_vec_add_parallel();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_vec_add_parallel Regression Test (F1)\n");
	int rc = kernel_vec_add_parallel();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

