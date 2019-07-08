/******************************************************************************/
/* Dram load store test.                                                      */
/* Does not execute any kernels on the tile group                             */
/* Simply stores a large array into DRAM and loads it back to compare.        */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/dram_load_store/   */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_dram_load_store.h"

#define TEST_NAME "test_dram_load_store"
#define ALLOC_NAME "default_allocator"


int kernel_dram_load_store() {
	bsg_pr_test_info("Running the CUDA DRAM Load Store test.\n\n");
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
                                                    "/dram_load_store/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return rc;
	}


        /**********************************************************************/
	/* Allocate memory on the device for A.                               */
        /**********************************************************************/
	uint32_t N = 4096;

	eva_t A_device; 
	rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to allocate memory on device.\n");
		return rc;
	}


        /**********************************************************************/
        /* Allocate memory on the host for A                                  */
        /* Initialize A_host_in with random values.                           */
        /* Set all elements in A_host_out to zero.                            */
        /**********************************************************************/
	uint32_t A_host_in[N]; 
	uint32_t A_host_out[N];
	for (int i = 0; i < N; i++) { 
		A_host_in[i] = rand() & 0xFFFF;
		A_host_out[i] = 0;
	}


        /**********************************************************************/
       	/* Copy A from host onto device DRAM.                                 */
        /**********************************************************************/
	void *dst = (void *) ((intptr_t) A_device);
	void *src = (void *) &A_host_in[0];
	rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);	
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to copy memory to device.\n");
		return rc;
	}


        /**********************************************************************/
	/* Copy A from device back into host.                                 */
        /**********************************************************************/
	src = (void *) ((intptr_t) A_device);
	dst = (void *) &A_host_out[0];
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
	int mismatch = 0; 
	for (int i = 0; i < N; i++) {
		if (A_host_in[i] != A_host_out[i]) {
			bsg_pr_err(BSG_RED("Mismatch: ") "A[%d] =  0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n",
                                                         i,
                                                         A_host_in[i],
                                                         A_host_out[i]);
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
	bsg_pr_test_info("test_dram_load_store Regression Test (COSIMULATION)\n");
	int rc = kernel_dram_load_store();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_dram_load_store Regression Test (F1)\n");
	int rc = kernel_dram_load_store();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

