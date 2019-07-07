/******************************************************************************/
/* A[N] + B[N] --> C[N]                                                       */
/* Runs the floating point vector addition on one 2x2 tile group              */
/* Grid dimensions are prefixed at 1x1. --> block_size_x is set to N.         */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/float_vec_add/     */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_float_vec_add.h"

#define TEST_NAME "test_float_vec_add"
#define ALLOC_NAME "default_allocator"


void host_float_vec_add (float *A, float *B, float *C, int N) { 
	for (int i = 0; i < N; i ++) { 
		C[i] = A[i] + B[i];
	}
	return;
}


int kernel_float_vec_add () {
	bsg_pr_test_info("Running the CUDA Floating Point Vector Addition "
                         "Kernel on a 1x1 grid of 2x2 tile group.\n\n");
	int rc;

	srand(time); 


        /**********************************************************************/
        /* Define path to binary.                                             */
        /* Initialize device, load binary and unfreeze tiles.                 */
        /**********************************************************************/
	hb_mc_device_t device;
	rc = hb_mc_device_init(&device, TEST_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize device.\n");
		return rc;
	}


	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime"
                                                    "/float_vec_add/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return rc;
	}


        /**********************************************************************/
	/* Allocate memory on the device for A, B and C.                      */
        /**********************************************************************/
	uint32_t N = 64;

	hb_mc_eva_t A_device, B_device, C_device; 
	rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


	rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B_device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


	rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &C_device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


        /**********************************************************************/
        /* Allocate memory on the host for A & B                              */
        /* and initialize with random values.                                 */
        /**********************************************************************/
	float A_host[N]; 
	float B_host[N]; 
	for (int i = 0; i < N; i++) { /* fill A with arbitrary data */
		A_host[i] = (float)rand() / (float)(RAND_MAX / 0xFFFF);
		B_host[i] =  (float)rand() / (float)(RAND_MAX / 0xFFFF);
	}


        /**********************************************************************/
	/* Copy A & B from host onto device DRAM.                             */
        /**********************************************************************/
	void *dst = (void *) ((intptr_t) A_device);
	void *src = (void *) &A_host[0];
	rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);	
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory to device.\n");
		return rc;
	}


	dst = (void *) ((intptr_t) B_device);
	src = (void *) &B_host[0];
	rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory to device.\n");
		return rc;
	}


        /**********************************************************************/
	/* Define block_size_x/y: amount of work for each tile group          */
	/* Define tg_dim_x/y: number of tiles in each tile group              */
	/* Calculate grid_dim_x/y: number of                                  */
        /* tile groups needed based on block_size_x/y                         */
        /**********************************************************************/
	uint32_t block_size_x = N;

	hb_mc_dimension_t tg_dim = { .x = 2, .y = 2}; 

	hb_mc_dimension_t grid_dim = { .x = 1, .y = 1}; 


        /**********************************************************************/
	/* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
	int argv[5] = {A_device, B_device, C_device, N, block_size_x};

	
        /**********************************************************************/
	/* Enquque grid of tile groups, pass in grid and tile group dimensions*/
        /* kernel name, number and list of input arguments                    */
        /**********************************************************************/
	rc = hb_mc_application_init (&device, grid_dim, tg_dim, "kernel_float_vec_add", 5, argv);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize grid.\n");
		return rc;
	}


        /**********************************************************************/
	/* Launch and execute all tile groups on device and wait for finish.  */ 
        /**********************************************************************/
	rc = hb_mc_device_tile_groups_execute(&device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to execute tile groups.\n");
		return rc;
	}


        /**********************************************************************/
	/* Copy result matrix back from device DRAM into host memory.         */
        /**********************************************************************/
	float C_host[N];
	src = (void *) ((intptr_t) C_device);
	dst = (void *) &C_host[0];
	rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory from device.\n");
		return rc;
	}


        /**********************************************************************/
        /* Freeze the tiles and memory manager cleanup.                       */
        /**********************************************************************/
	rc = hb_mc_device_finish(&device); 
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to de-initialize device.\n");
		return rc;
	}


        /**********************************************************************/
	/* Calculate the expected result using host code and compare.         */ 
        /**********************************************************************/
	float C_expected[N]; 
	host_float_vec_add (A_host, B_host, C_expected, N); 


	int mismatch = 0; 
	for (int i = 0; i < N; i++) {
		if (A_host[i] + B_host[i] != C_host[i]) {
			bsg_pr_err(BSG_RED("Mismatch: ") "C[%d]:  %f + %f = %f\t Expected: %f\n",
                                           i,
                                           A_host[i],
                                           B_host[i],
                                           C_host[i],
                                           C_expected[i]);
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
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_float_vec_add Regression Test (COSIMULATION)\n");
	int rc = kernel_float_vec_add();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_float_vec_add Regression Test (F1)\n");
	int rc = kernel_float_vec_add();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

