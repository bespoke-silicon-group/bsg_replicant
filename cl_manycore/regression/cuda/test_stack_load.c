#include "test_stack_load.h"

#define TEST_NAME "test_stack_load"
#define ALLOC_NAME "default_allocator"

/*!
 * Runs the stack_load test on a grid of one tile group
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/stack_load/ Manycore binary in the BSG Manycore bitbucket repository.  
*/


#define	NUM_ARGS	15		// Number of arguments passed to kernel
#define	SUM_ARGS	120		// Sum of arguments passed to kernel


int kernel_stack_load () {
	bsg_pr_test_info("Running the CUDA Stack Load Kernel on a 1x1 grid of 2x2 tile group.\n\n");
	int rc;

	/*****************************************************************************************************************
	* Define the dimension of tile pool.
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	hb_mc_device_t device;
	hb_mc_dimension_t mesh_dim = { .x = 4, .y = 4 }; 
	rc = hb_mc_device_init(&device, TEST_NAME, 0,  mesh_dim);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize device.\n");
		return HB_MC_FAIL;
	}


	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/stack_load/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return HB_MC_FAIL;
	}


	/*****************************************************************************************************************
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 
	hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 }; 


	/*****************************************************************************************************************
	* Allocate memory on the device for sum of input arguments, one element for each tile.
	******************************************************************************************************************/
	eva_t sum_device;
	rc = hb_mc_device_malloc(&device, tg_dim.x * tg_dim.y * sizeof (uint32_t), &sum_device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return HB_MC_FAIL;
	}


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel. {Num of arguments, Sum of arguments, arguments}
	******************************************************************************************************************/
	int argv[NUM_ARGS + 2] = {sum_device, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};


	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	rc = hb_mc_grid_init (&device, grid_dim, tg_dim, "kernel_stack_load", NUM_ARGS + 1, argv);
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
	* Copy result sum back from device DRAM into host memory. 
	******************************************************************************************************************/
	uint32_t sum_host[tg_dim.x * tg_dim.y];
	void *src = (void *) ((intptr_t) sum_device);
	void *dst = (void *) &sum_host[0];
	rc = hb_mc_device_memcpy (&device, (void *) dst, src, (tg_dim.x * tg_dim.y) * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy sum_device to the host */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory from device.\n");
		return HB_MC_FAIL;
	}



	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	rc = hb_mc_device_finish(&device); 
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to de-initialize device.\n");
		return HB_MC_FAIL;
	}


	/*****************************************************************************************************************
	* Compare the expected sum and the manycore sum. 
	******************************************************************************************************************/
	int mismatch = 0;
	for (int y = 0; y < tg_dim.y; y ++) { 
		for (int x = 0; x < tg_dim.x; x ++) { 
			if (sum_host[y * tg_dim.x + x] != SUM_ARGS) { 
				bsg_pr_err(BSG_RED("Mismatch: ") "sum[%d][%d] = %d\tExpected %d.\n", y, x, sum_host[y * tg_dim.x + x], SUM_ARGS);
				mismatch = 0; 
			}
		}
	}


	if (mismatch) { 
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_stack_load Regression Test (COSIMULATION)\n");
	int rc = kernel_stack_load();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_stack_load Regression Test (F1)\n");
	int rc = kernel_stack_load();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

