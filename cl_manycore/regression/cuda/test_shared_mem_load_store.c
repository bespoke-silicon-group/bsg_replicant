#include "test_shared_mem_load_store.h"

#define TEST_NAME "test_shared_mem_load_store"
#define ALLOC_NAME "default_allocator"

/*!
 * Runs a tile group shared memory load/store kernel. Loads a M * N matrix into tile group shared memory and stores it back to another location.
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_y/x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/shared_mem_load_store/ Manycore binary in the BSG Manycore bitbucket repository.  
*/


int kernel_shared_mem_load_store () {
	bsg_pr_test_info("Running the CUDA Shared Memory Load Store Kernel.\n\n");
	int rc;

	srand(time); 


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
		return rc;
	}


	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/shared_mem_load_store/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Allocate memory on the device for A, B and C.
	******************************************************************************************************************/
	uint32_t M = 64;
	uint32_t N = 64;

	eva_t A_in_device, A_out_device; 
	rc = hb_mc_device_malloc(&device, M * N * sizeof(uint32_t), &A_in_device); /* allocate A_in[M][N] on the device */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


	rc = hb_mc_device_malloc(&device, N * N * sizeof(uint32_t), &A_out_device); /* allocate A_out[M][N] on the device */
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Allocate memory on the host for A_in and initialize with random values.
	******************************************************************************************************************/
	uint32_t A_in_host[M * N]; /* allocate A[M][N] on the host */ 
	for (int i = 0; i < M * N; i++) { /* fill A with arbitrary data */
		A_in_host[i] = i; // rand() & 0xFFFF;
	}



	/*****************************************************************************************************************
	* Copy A_in from host onto device DRAM.
	******************************************************************************************************************/
	void *dst = (void *) ((intptr_t) A_in_device);
	void *src = (void *) &A_in_host[0];
	rc = hb_mc_device_memcpy (&device, dst, src, (M * N) * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy A_in to the device  */	
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory to device.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Define block_size_x/y: amount of work for each tile group
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	uint32_t block_size_y = 32;
	uint32_t block_size_x = 16;

	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 };

	hb_mc_dimension_t grid_dim = { .x = N / block_size_x, .y = M / block_size_y };


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[6] = {A_in_device, A_out_device, M, N, block_size_y, block_size_x};

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	rc = hb_mc_grid_init (&device, grid_dim, tg_dim, "kernel_shared_mem_load_store", 6, argv);
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
	uint32_t A_out_host[M * N];
	src = (void *) ((intptr_t) A_out_device);
	dst = (void *) &A_out_host[0];
	rc = hb_mc_device_memcpy (&device, (void *) dst, src, (M * N) * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST); /* copy A_out to the host */
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


	// Compare matrices
	int mismatch = 0; 
	for (int y = 0; y < M; y ++) { 
		for (int x = 0; x < N; x ++) { 
			if ( A_in_host[y * N + x] != A_out_host[y * N + x]) { 
				bsg_pr_err(BSG_RED("Mismatch: ") "A[%d][%d] = %d\t Expected: %d.\n", y, x, A_out_host[y * N + x], A_in_host[y * N + x]); 
				mismatch = 1;
			}
		}
	}


	if (mismatch) { 
		bsg_pr_err(BSG_RED("Matrix mismatch.\n"));
		return HB_MC_FAIL;
	}
	bsg_pr_test_info(BSG_GREEN("Matrix Match.\n"));
	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_shared_mem_load_store Regression Test (COSIMULATION)\n");
	int rc = kernel_shared_mem_load_store();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_shared_mem_load_store Regression Test (F1)\n");
	int rc = kernel_shared_mem_load_store();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

