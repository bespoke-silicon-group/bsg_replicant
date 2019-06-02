#include "test_vec_add_parallel_multi_grid.h"

#define TEST_NAME "test_vec_add_parallel_multi_grid"

/*!
 * Runs two separate vector addition kernels a two grids of 2x2 tile groups. A1[N] + B1[N] --> C1[N], A2[M] + B2[M] --> C2[M]
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_parallel_multi_grid/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore bitbucket repository.  
*/


void host_vec_add (int *A, int *B, int *C, int N) { 
	for (int i = 0; i < N; i ++) { 
		C[i] = A[i] + B[i];
	}
	return;
}


int kernel_vec_add_parallel_multi_grid () {
	fprintf(stderr, "Running Two Separate CUDA Vector Addition Kernels on two grids of 2x2 tile groups.\n\n");

	srand(time); 


	/*****************************************************************************************************************
	* Define the dimension of tile pool.
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	device_t device;
	hb_mc_dimension_t mesh_dim = { .x = 4, .y = 4 } ;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/vec_add_parallel_multi_grid/main.riscv";

	hb_mc_device_init(&device, elf, TEST_NAME, 0, mesh_dim);




	/*****************************************************************************************************************
	* Allocate memory on the device for A1, B1 and C1.
	******************************************************************************************************************/
	uint32_t N = 1024;

	eva_t A1_device, B1_device, C1_device; 
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A1_device); /* allocate A1[N] on the device */
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B1_device); /* allocate B1[N] on the device */
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &C1_device); /* allocate C1[N] on the device */


	/*****************************************************************************************************************
	* Allocate memory on the device for A2, B2 and C2.
	******************************************************************************************************************/
	uint32_t M = 512;

	eva_t A2_device, B2_device, C2_device; 
	hb_mc_device_malloc(&device, M * sizeof(uint32_t), &A2_device); /* allocate A2[M] on the device */
	hb_mc_device_malloc(&device, M * sizeof(uint32_t), &B2_device); /* allocate B2[M] on the device */
	hb_mc_device_malloc(&device, M * sizeof(uint32_t), &C2_device); /* allocate C2[M] on the device */



	/*****************************************************************************************************************
	* Allocate memory on the host for A1 & B1 and initialize with random values.
	******************************************************************************************************************/
	uint32_t A1_host[N]; /* allocate A1[N] on the host */ 
	uint32_t B1_host[N]; /* allocate B1[N] on the host */
	for (int i = 0; i < N; i++) { /* fill A1 & B1 with arbitrary data */
		A1_host[i] = rand() & 0xFFFF;
		B1_host[i] = rand() & 0xFFFF;
	}


	/*****************************************************************************************************************
	* Allocate memory on the host for A2 & B2 and initialize with random values.
	******************************************************************************************************************/
	uint32_t A2_host[M]; /* allocate A2[M] on the host */ 
	uint32_t B2_host[M]; /* allocate B2[M] on the host */
	for (int i = 0; i < M; i++) { /* fill A2 & B2 with arbitrary data */
		A2_host[i] = rand() & 0xFFFF;
		B2_host[i] = rand() & 0xFFFF;
	}



	/*****************************************************************************************************************
	* Copy A1 & B1 from host onto device DRAM.
	******************************************************************************************************************/
	void *dst = (void *) ((intptr_t) A1_device);
	void *src = (void *) &A1_host[0];
	hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A1 to the device  */	
	dst = (void *) ((intptr_t) B1_device);
	src = (void *) &B1_host[0];
	hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B1 to the device */ 



	/*****************************************************************************************************************
	* Copy A2 & B2 from host onto device DRAM.
	******************************************************************************************************************/
	dst = (void *) ((intptr_t) A2_device);
	src = (void *) &A2_host[0];
	hb_mc_device_memcpy (&device, dst, src, M * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A2 to the device  */	
	dst = (void *) ((intptr_t) B2_device);
	src = (void *) &B2_host[0];
	hb_mc_device_memcpy (&device, dst, src, M * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B2 to the device */ 




	/*****************************************************************************************************************
	* Define block_size_x/y: amount of work for each tile group
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	uint32_t block_size_x1 = 64;
	uint32_t block_size_x2 = 256;

	hb_mc_dimension_t tg_dim_1 = { .x = 2, .y = 2 } ;
	hb_mc_dimension_t tg_dim_2 = { .x = 2, .y = 2 } ;


	hb_mc_dimension_t grid_dim_1 = { .x = N / block_size_x1, .y = 1 }; 
	hb_mc_dimension_t grid_dim_2 = { .x = M / block_size_x2, .y = 2 };



	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv1[5] = {A1_device, B1_device, C1_device, N, block_size_x1};
	int argv2[5] = {A2_device, B2_device, C2_device, M, block_size_x2};

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	hb_mc_grid_init (&device, grid_dim_1, tg_dim_1, "kernel_vec_add_parallel_multi_grid", 5, argv1);
	hb_mc_grid_init (&device, grid_dim_2, tg_dim_2, "kernel_vec_add_parallel_multi_grid", 5, argv2);

	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	hb_mc_device_tile_groups_execute(&device);
	


	/*****************************************************************************************************************
	* Copy result matrix back from device DRAM into host memory for grid 1. 
	******************************************************************************************************************/
	uint32_t C1_host[N];
	src = (void *) ((intptr_t) C1_device);
	dst = (void *) &C1_host[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C1 to the host */


	/*****************************************************************************************************************
	* Copy result matrix back from device DRAM into host memory for grid 2.
	******************************************************************************************************************/
	uint32_t C2_host[N];
	src = (void *) ((intptr_t) C2_device);
	dst = (void *) &C2_host[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, M * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C2 to the host */



	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	hb_mc_device_finish(&device); 


	/*****************************************************************************************************************
	* Calculate the expected result using host code and compare the results. 
	******************************************************************************************************************/
	uint32_t C1_expected[N]; 
	host_vec_add (A1_host, B1_host, C1_expected, N); 

	uint32_t C2_expected[M]; 
	host_vec_add (A2_host, B2_host, C2_expected, M); 



	int mismatch = 0; 


	fprintf (stderr, "\n\n\nGrid 1 Results:\n");
	for (int i = 0; i < N; i++) {
		if (A1_host[i] + B1_host[i] == C1_host[i]) {
			fprintf(stderr, "Success -- C1[%d]:  0x%x + 0x%x = 0x%x\t Expected: 0x%x\n", i , A1_host[i], B1_host[i], C1_host[i], C1_expected[i]);
		}
		else {
			fprintf(stderr, "Failed  -- C1[%d]:  0x%x + 0x%x = 0x%x\t Expected: 0x%x\n", i , A1_host[i], B1_host[i], C1_host[i], C1_expected[i]);
			mismatch = 1;
		}
	} 


	fprintf (stderr, "\n\n\nGrid 2 Results:\n");
	for (int i = 0; i < M; i++) {
		if (A2_host[i] + B2_host[i] == C2_host[i]) {
			fprintf(stderr, "Success -- C2[%d]:  0x%x + 0x%x = 0x%x\t Expected: 0x%x\n", i , A2_host[i], B2_host[i], C2_host[i], C2_expected[i]);
		}
		else {
			fprintf(stderr, "Failed  -- C2[%d]:  0x%x + 0x%x = 0x%x\t Expected: 0x%x\n", i , A2_host[i], B2_host[i], C2_host[i], C2_expected[i]);
			mismatch = 1;
		}
	} 


	if (mismatch) { 
		return HB_MC_FAIL;
	}
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

