#include "test_empty_parallel.h"

/*!
 * Runs 5 empty kernels in 2x2 tile groups on a 4x4 mesh. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/empty_parallel/ Manycore binary in the dev_cuda_tile_group_refactored branch of the BSG Manycore github repository.  
*/
int kernel_empty_parallel () {
	fprintf(stderr, "Running the CUDA Empyt Kernel on a 2x2 tile group.\n\n");

	device_t device;
	uint8_t mesh_dim_x = 4;
	uint8_t mesh_dim_y = 4;
	uint8_t mesh_origin_x = 0;
	uint8_t mesh_origin_y = 1;
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/empty_parallel/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, mesh_dim_x, mesh_dim_y, mesh_origin_x, mesh_origin_y);

	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	int argv[1];

	hb_mc_tile_group_enqueue (&device, tg_dim_x, tg_dim_y, "kernel_empty", 0, argv);
	hb_mc_tile_group_enqueue (&device, tg_dim_x, tg_dim_y, "kernel_empty", 0, argv);
	hb_mc_tile_group_enqueue (&device, tg_dim_x, tg_dim_y, "kernel_empty", 0, argv);
	hb_mc_tile_group_enqueue (&device, tg_dim_x, tg_dim_y, "kernel_empty", 0, argv);
	hb_mc_tile_group_enqueue (&device, tg_dim_x, tg_dim_y, "kernel_empty", 0, argv);


	hb_mc_device_tile_groups_execute(&device);
	
	hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */

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
	bsg_pr_test_info("test_empty_parlalel Regression Test (F1)\n");
	int rc = kernel_empty_parallel();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

