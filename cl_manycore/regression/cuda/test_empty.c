#include "test_empty.h"


#define TEST_NAME "test_empty"

/*!
 * Runs an empty kernel on a 2x2 tile group. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/empty/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore github repository.  
*/

int kernel_empty () {
	fprintf(stderr, "Running the CUDA Empty Kernel on a 2x2 tile group.\n\n");


	/*****************************************************************************************************************
	* Define the dimension of tile pool.
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	device_t device;
	hb_mc_dimension_t mesh_dim = {.x = 4, .y = 4};
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/empty/main.riscv";

	hb_mc_device_init(&device, elf, TEST_NAME, 0,  mesh_dim);


	/*****************************************************************************************************************
	* Define grid_dim_x/y: total number of tile groups
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	hb_mc_dimension_t grid_dim = {.x = 1, .y = 1};
	hb_mc_dimension_t tg_dim = {.x = 2, .y = 2};


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[1];


	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	hb_mc_grid_init (&device, grid_dim, tg_dim, "kernel_empty", 0, argv);
	

	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	hb_mc_device_tile_groups_execute(&device);
	

	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */

	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_empty Regression Test (COSIMULATION)\n");
	int rc = kernel_empty();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_empty Regression Test (F1)\n");
	int rc = kernel_empty();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

