#include "test_empty.h"

/*!
 * Runs an empty kernel on a 2x2 tile group. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/empty/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore github repository.  
*/
int kernel_empty () {
	fprintf(stderr, "Running the CUDA Empty Kernel on a 2x2 tile group.\n\n");

	device_t device;
	uint8_t mesh_dim_x = 4;
	uint8_t mesh_dim_y = 4;
	uint8_t mesh_origin_x = 0;
	uint8_t mesh_origin_y = 1;
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/empty/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, mesh_dim_x, mesh_dim_y, mesh_origin_x, mesh_origin_y);

	uint8_t grid_dim_x = 1;
	uint8_t grid_dim_y = 1;
	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	int argv[1];

	hb_mc_grid_init (&device, grid_dim_x, grid_dim_y, tg_dim_x, tg_dim_y, "kernel_empty", 0, argv);
	
	hb_mc_device_tile_groups_execute(&device);
	
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

