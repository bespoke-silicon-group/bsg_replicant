#include "test_memset.h"

#define TEST_NAME "test_memset"
#define ALLOC_NAME "default_allocator"
#define TEST_BYTE 0xcd

/*!
 * Runs a memset kernel on a 2x2 tile group. 
 * Device allcoates memory on device and uses memset to set to a prefixed valu.
 * Device then calls an empty kernel and loads back the meomry to compare.
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/memset/ Manycore binary in the BSG Manycore bitbucket repository.  
*/


int kernel_memset () {
	bsg_pr_test_info("Running the CUDA Memset Kernel on a grid of one 2x2 tile group.\n\n");
	int rc;

	/*****************************************************************************************************************
	* Define path to binary.
	* Initialize device, load binary and unfreeze tiles.
	******************************************************************************************************************/
	hb_mc_device_t device;
	rc = hb_mc_device_init(&device, TEST_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize device.\n");
		return rc;
	}

	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/memset/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return rc;
	}



	/*****************************************************************************************************************
	* Allocate memory on the device for A and set it to TEST_VAL
	******************************************************************************************************************/
	uint32_t N = 64;
	eva_t A_device; 
	rc = hb_mc_device_malloc(&device, N * sizeof(uint8_t), &A_device); /* allocate A_ptr on the device */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


	rc = hb_mc_device_memset(&device, &A_device, TEST_BYTE, N * sizeof(uint8_t));
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to set memory on device.\n");
		return rc;
	} 


	/*****************************************************************************************************************
	* Define block_size_x/y: amount of work for each tile group
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 

	hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[1];

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	rc = hb_mc_application_init (&device, grid_dim, tg_dim, "kernel_memset", 0, argv);
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
	* Copy result matrix back from device DRAM into host memory. 
	******************************************************************************************************************/
	uint8_t A_host[N];
	void *src = (void *) ((intptr_t) A_device);
	void *dst = (void *) &A_host[0];
	rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint8_t), HB_MC_MEMCPY_TO_HOST);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory from device.\n");
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


	int mismatch = 0; 
	for (int i = 0; i < N; i++) {
		if (A_host[i] != TEST_BYTE) { 
			bsg_pr_err(BSG_RED("Mismatch") ": -- A[%d] = 0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n", i , A_host[i], TEST_BYTE);
			mismatch = 1;
		}
	} 

	if (mismatch) { 
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
        // We aren't passed command line arguments directly so we parse them
        // from *args. args is a string from VCS - to pass a string of arguments
        // to args, pass c_args to VCS as follows: +c_args="<space separated
        // list of args>"
        int argc = get_argc(args);
        char *argv[argc];
        get_argv(args, argc, argv);

#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_memset Regression Test (COSIMULATION)\n");
	int rc = kernel_memset();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info("test_memset Regression Test (F1)\n");
	int rc = kernel_memset();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

