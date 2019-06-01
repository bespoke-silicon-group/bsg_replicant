#include "test_vec_add_shared_mem.h"

#define TEST_NAME "test_vec_add_shared_mem"

/*!
 * Runs the vector addition with shared memory on a grid of 2x2 tile groups. A[N] + B[N] --> C[N]
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_shared_mem/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore bitbucket repository.  
*/


void host_vec_add (int *A, int *B, int *C, int N) { 
	for (int i = 0; i < N; i ++) { 
		C[i] = A[i] + B[i];
	}
	return;
}


int kernel_vec_add_shared_mem () {
	fprintf(stderr, "Running the CUDA Vector Addition Shared Memory Kernel on a grid of 2x2 tile groups.\n\n");

	srand(time); 


	/*****************************************************************************************************************
	* Define the dimension of tile pool.
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	device_t device;
	hb_mc_dimension_t mesh_dim = { .x = 4, .y = 4 };
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/vec_add_shared_mem/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, TEST_NAME, 0, mesh_dim);




	/*****************************************************************************************************************
	* Allocate memory on the device for A, B and C.
	******************************************************************************************************************/
	uint32_t N = 256;

	eva_t A_device, B_device, C_device; 
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device); /* allocate A[N] on the device */
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B_device); /* allocate B[N] on the device */
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &C_device); /* allocate C[N] on the device */



	/*****************************************************************************************************************
	* Allocate memory on the host for A & B and initialize with random values.
	******************************************************************************************************************/
	uint32_t A_host[N]; /* allocate A[N] on the host */ 
	uint32_t B_host[N]; /* allocate B[N] on the host */
	for (int i = 0; i < N; i++) { /* fill A with arbitrary data */
		A_host[i] = i;     //rand() & 0xFFFF;
		B_host[i] = 2 * i; //rand() & 0xFFFF;
	}



	/*****************************************************************************************************************
	* Copy A & B from host onto device DRAM.
	******************************************************************************************************************/
	void *dst = (void *) ((intptr_t) A_device);
	void *src = (void *) &A_host[0];
	hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A to the device  */	
	dst = (void *) ((intptr_t) B_device);
	src = (void *) &B_host[0];
	hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B to the device */ 



	/*****************************************************************************************************************
	* Define block_size_x/y: amount of work for each tile group
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	uint32_t block_size_x = 16;

	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 

	hb_mc_dimension_t grid_dim = { .x = N / block_size_x , .y = 1 };


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[5] = {A_device, B_device, C_device, N, block_size_x};

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	hb_mc_grid_init (&device, grid_dim, tg_dim, "kernel_vec_add_shared_mem", 5, argv);

	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	hb_mc_device_tile_groups_execute(&device);
	


	/*****************************************************************************************************************
	* Copy result matrix back from device DRAM into host memory. 
	******************************************************************************************************************/
	uint32_t C_host[N];
	src = (void *) ((intptr_t) C_device);
	dst = (void *) &C_host[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C to the host */



	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	hb_mc_device_finish(&device); 


	/*****************************************************************************************************************
	* Calculate the expected result using host code and compare the results. 
	******************************************************************************************************************/
	uint32_t C_expected[N]; 
	host_vec_add (A_host, B_host, C_expected, N); 


	int mismatch = 0; 
	for (int i = 0; i < N; i++) {
		if (A_host[i] + B_host[i] == C_host[i]) {
			fprintf(stderr, "Success -- C[%d]: 0x%x + 0x%x = 0x%x\t Expected: 0x%x\n", i , A_host[i], B_host[i], C_host[i], C_expected[i]);
		}
		else {
			fprintf(stderr, "Failed  -- C[%d]:  0x%x + 0x%x = 0x%x\t Expected: 0x%x\n", i , A_host[i], B_host[i], C_host[i], C_expected[i]);
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
	bsg_pr_test_info("test_vec_add_shared_mem Regression Test (COSIMULATION)\n");
	int rc = kernel_vec_add_shared_mem();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_vec_add_shared_mem Regression Test (F1)\n");
	int rc = kernel_vec_add_shared_mem();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

