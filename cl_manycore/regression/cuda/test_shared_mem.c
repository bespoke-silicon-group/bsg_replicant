#include "test_shared_mem.h"

/*!
 * Runs the shared_mem test on a grid of one tile group
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/shared_mem/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore bitbucket repository.  
*/

#define N 16


int kernel_shared_mem () {
	fprintf(stderr, "Running the CUDA Shared Memory  Kernel on a 1x1 grid of 2x2 tile group.\n\n");

	srand(time); 


	/*****************************************************************************************************************
	* Define the dimension of tile pool.
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	device_t device;
	uint8_t mesh_dim_x = 4;
	uint8_t mesh_dim_y = 4;
	uint8_t mesh_origin_x = 0;
	uint8_t mesh_origin_y = 1;
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/shared_mem/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, mesh_dim_x, mesh_dim_y, mesh_origin_x, mesh_origin_y);


	/*****************************************************************************************************************
	* Allocate memory on the device for A.
	******************************************************************************************************************/
	eva_t A_device; 
	hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device); /* allocate A[N] on the device */


	/*****************************************************************************************************************
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	uint32_t grid_dim_x = 1;
	uint32_t grid_dim_y = 1;


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[2] = {A_device, N};

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	hb_mc_grid_init (&device, grid_dim_x, grid_dim_y, tg_dim_x, tg_dim_y, "kernel_shared_mem", 2, argv);

	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	hb_mc_device_tile_groups_execute(&device);
	

	/*****************************************************************************************************************
	* Copy result array back from device DRAM into host memory. 
	******************************************************************************************************************/
	uint32_t A_host[N];
	void *src = (void *) ((intptr_t) A_device);
	void *dst = (void *) &A_host[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host */


	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	hb_mc_device_finish(&device); 



	/*****************************************************************************************************************
	* Compare the results. 
	******************************************************************************************************************/	
	int mismatch = 0;
	for (int i = 0; i < N; i ++) { 
		if (A_host[i] == i) { 
			fprintf (stderr, "Success: A[%d] = %d\t Expected: %d.\n", i, A_host[i], i); 
		}
		else 
		{
			fprintf (stderr, "Failed : A_host[%d] = %d\t Expected: %d.\n", i, A_host[i], i);
			mismatch = 1 ;
		}
	} 


	if (mismatch) { 
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_shared_mem Regression Test (COSIMULATION)\n");
	int rc = kernel_shared_mem();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_shared_mem Regression Test (F1)\n");
	int rc = kernel_shared_mem();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

