#include "test_shared_mem.h"

#define TEST_NAME "test_shared_mem"
#define ALLOC_NAME "default_allocator"

/*!
 * Runs the shared_mem test on a grid of one tile group
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/shared_mem/ Manycore binary in the BSG Manycore bitbucket repository.  
*/

// Maximum N that passed the test with 2x2 tile group dimensions is 2048
#define N 256


int kernel_shared_mem () {
	bsg_pr_test_info("Running the CUDA Shared Memory  Kernel on a 1x1 grid of 2x2 tile group.\n\n");
	int rc;

	srand(time); 


	/*****************************************************************************************************************
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	hb_mc_device_t device;
	rc =hb_mc_device_init(&device, TEST_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize device.\n");
		return rc;
	}


	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/shared_mem/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Allocate memory on the device for A.
	******************************************************************************************************************/
	eva_t A_device; 
	rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device); /* allocate A[N] on the device */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allcoate memory on device.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 };

	hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[2] = {A_device, N};

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	rc = hb_mc_application_init (&device, grid_dim, tg_dim, "kernel_shared_mem", 2, argv);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize grid.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	rc = hb_mc_device_tile_groups_execute(&device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to execute tile groups.\n");
		return rc;
	}
	

	/*****************************************************************************************************************
	* Copy result array back from device DRAM into host memory. 
	******************************************************************************************************************/
	uint32_t A_host[N];
	void *src = (void *) ((intptr_t) A_device);
	void *dst = (void *) &A_host[0];
	rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST); /* copy A to the host */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory to device.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	rc = hb_mc_device_finish(&device); 
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to de-initialize device.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Compare the results. 
	******************************************************************************************************************/	
	int mismatch = 0;
	for (int i = 0; i < N; i ++) { 
		if (A_host[i] != i) { 
			bsg_pr_err(BSG_RED("Mismatch: ") "A_host[%d] = %d\t Expected: %d.\n", i, A_host[i], i);
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
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
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

