#include "test_empty_parallel.h"

#define TEST_NAME "test_empty_parallel"
#define ALLOC_NAME "default_allocator"

/*!
 * Runs an empty kernel on a 4x2 grid of 2x2 tile groups. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/empty_parallel/ Manycore binary in the BSG Manycore github repository.  
*/

int kernel_empty_parallel () {
	bsg_pr_test_info("Running the CUDA Empty Parallel Kernel on a 4x2 grid of 2x2 tile groups.\n\n");
	int rc;


	/*****************************************************************************************************************
	* Define the dimension of tile pool.
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	device_t device;
	hb_mc_dimension_t mesh_dim = { .x = 4, .y = 4}; 
	rc = hb_mc_device_init(&device, TEST_NAME, 0, mesh_dim);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize device.\n");
		return HB_MC_FAIL;
	}


	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/empty_parallel/main.riscv";
	rc =hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return HB_MC_FAIL;
	}


	/*****************************************************************************************************************
	* Define grid_dim_x/y: total number of tile groups
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	hb_mc_dimension_t grid_dim = { .x = 4, .y = 2}; 
	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2}; 


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[1];


	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	rc = hb_mc_grid_init (&device, grid_dim, tg_dim, "kernel_empty", 0, argv);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize grid.\n");
		return HB_MC_FAIL;
	}
	

	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	rc = hb_mc_device_tile_groups_execute(&device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to execute tile groups.\n");
		return HB_MC_FAIL;
	}
	

	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	rc = hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to de-initialize device.\n");
		return HB_MC_FAIL;
	}

	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_empty_parallel Regression Test (COSIMULATION)\n");
	int rc = kernel_empty_parallel();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_empty_parallel Regression Test (F1)\n");
	int rc = kernel_empty_parallel();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

