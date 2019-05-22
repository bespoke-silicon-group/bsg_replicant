#include "test_dram.h"

/*!
 * Runs the dram store test a grid of 2x2 tile groups. Tiles allocate space on dram and fill it, and return the pointer to host. Host then picks up the array and compares.
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_dram/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore bitbucket repository.  
*/


int kernel_dram () {
	fprintf(stderr, "Running the CUDA Vector Addition Kernel on a grid of 2x2 tile groups.\n\n");

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
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/dram/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, mesh_dim_x, mesh_dim_y, mesh_origin_x, mesh_origin_y);




	/*****************************************************************************************************************
	* Allocate memory on the device for A_ptr.
	******************************************************************************************************************/
	eva_t A_ptr_device; 
	hb_mc_device_malloc(&device, 1 * sizeof(uint32_t), &A_ptr_device); /* allocate A_ptr on the device */


	/*****************************************************************************************************************
	* Define block_size_x/y: amount of work for each tile group
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	uint32_t N = 1024;
	uint32_t block_size_x = 64;

	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	uint32_t grid_dim_x = N / block_size_x;
	uint32_t grid_dim_y = 1;


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[2] = {A_ptr_device, block_size_x};

	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	hb_mc_grid_init (&device, grid_dim_x, grid_dim_y, tg_dim_x, tg_dim_y, "kernel_dram", 2, argv);

	/*****************************************************************************************************************
	* Launch and execute all tile groups on device and wait for all to finish. 
	******************************************************************************************************************/
	hb_mc_device_tile_groups_execute(&device);
	


	/*****************************************************************************************************************
	* Copy result matrix back from device DRAM into host memory. 
	******************************************************************************************************************/
	uint32_t A_ptr_host;
	void *src = (void *) ((intptr_t) A_ptr_device);
	void *dst = (void *) &A_ptr_host;
	hb_mc_device_memcpy (&device, (void *) dst, src, 1 * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A_ptr to the host */



	uint32_t A_host[N];
	src = (void *) ((intptr_t) A_ptr_host);
	dst = (void *) &A_host[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy A to the host using A_ptr*/




	/*****************************************************************************************************************
	* Freeze the tiles and memory manager cleanup. 
	******************************************************************************************************************/
	hb_mc_device_finish(&device); 


	int mismatch = 0; 
	for (int i = 0; i < N; i++) {
		if (A_host[i] == i) {
			fprintf(stderr, "Success -- A[%d] = 0x%x\t Expected: 0x%x\n", i , A_host[i], i);
		}
		else {
			fprintf(stderr, "Failed  -- A[%d] = 0x%x\t Expected: 0x%x\n", i , A_host[i], i);
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
	bsg_pr_test_info("test_dram Regression Test (COSIMULATION)\n");
	int rc = kernel_dram();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_dram Regression Test (F1)\n");
	int rc = kernel_dram();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

