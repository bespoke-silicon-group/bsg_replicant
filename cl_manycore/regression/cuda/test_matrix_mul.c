#include "test_matrix_mul.h"

/*!
 * Runs the matrix multiplication on a grid of 4 2x2 tile groups. 
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/matrix_mul/ Manycore binary in the dev_cuda_v4 branch of the BSG Manycore bitbucket repository.  
*/




void matrix_mult (uint32_t *A, uint32_t *B, uint32_t *C, int n) { 
	for (int y = 0; y < n; y ++) { 
		for (int x = 0; x < n; x ++) { 
			int res = 0;
			for (int k = 0; k < n; k++) { 
				res += A[y * n + k] * B[k * n + x];
			}
			C[y * n + x] = res;
		}
	}
	return;
}
				


int kernel_matrix_mul () {
	fprintf(stderr, "Running the CUDA Matrix Multiplication Kernel on a grid of 4 2x2 tile groups.\n\n");

	device_t device;
	uint8_t mesh_dim_x = 4;
	uint8_t mesh_dim_y = 4;
	uint8_t mesh_origin_x = 0;
	uint8_t mesh_origin_y = 1;
	eva_id_t eva_id = 0;
	char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_cuda_lite_runtime" "/matrix_mul/main.riscv";

	hb_mc_device_init(&device, eva_id, elf, mesh_dim_x, mesh_dim_y, mesh_origin_x, mesh_origin_y);

	uint32_t n = 16;
	uint32_t size_buffer = n * n; 
	eva_t A_device, B_device, C_device; 
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &A_device); /* allocate A on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &B_device); /* allocate B on the device */
	hb_mc_device_malloc(&device, size_buffer * sizeof(uint32_t), &C_device); /* allocate C on the device */

	uint32_t A_host[size_buffer]; /* allocate A on the host */ 
	uint32_t B_host[size_buffer]; /* allocate B on the host */
	srand(0);
	for (int i = 0; i < size_buffer; i++) { /* fill A and B with arbitrary data */
		A_host[i] = rand() % ((1 << 16) - 1); /* avoid overflow */
		B_host[i] = rand() % ((1 << 16) - 1); 
	}

	void *dst = (void *) ((intptr_t) A_device);
	void *src = (void *) &A_host[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy A1 to the device  */	
	dst = (void *) ((intptr_t) B_device);
	src = (void *) &B_host[0];
	hb_mc_device_memcpy (&device, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device); /* Copy B1 to the device */ 

	uint8_t grid_dim_x = 2;
	uint8_t grid_dim_y = 2;
	uint8_t tg_dim_x = 2;
	uint8_t tg_dim_y = 2;

	int argv[4] = {A_device, B_device, C_device, n};

	hb_mc_grid_init (&device, grid_dim_x, grid_dim_y, tg_dim_x, tg_dim_y, "kernel_matrix_mul", 4, argv);

	hb_mc_device_tile_groups_execute(&device);
	

	uint32_t C_result[size_buffer];
	src = (void *) ((intptr_t) C_device);
	dst = (void *) &C_result[0];
	hb_mc_device_memcpy (&device, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host); /* copy C to the host */

	hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */


	/* calculate expected results. */
	uint32_t C_expected[size_buffer]; 
	matrix_mult (A_host, B_host, C_expected, n); 



	int mismatch = 0; 
	for (int i = 0; i < size_buffer; i++) {
		if ( C_expected[i] != C_result[i]) {
			mismatch = 1;
			break; 
		}
	}

	fprintf(stderr, "Expected Result:\n");
	for (int y = 0; y < n; y++) { 
		for (int x = 0; x < n; x++) {
			fprintf(stderr, "%d\t", C_expected[y * n + x]);
		}
		fprintf(stderr, "\n");
	}
		
	fprintf(stderr, "Manycore Result:\n");
	for (int y = 0; y < n; y++) { 
		for (int x = 0; x < n; x++) {
			fprintf(stderr, "%d\t", C_result[y * n + x]);
		}
		fprintf(stderr, "\n");
	}

	if (mismatch) { 
		fprintf(stderr, "Failed: matrix mismatch.\n");
		return HB_MC_FAIL;
	}
	fprintf(stderr, "Success: matrix match.\n");
	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_matrix_mul Regression Test (COSIMULATION)\n");
	int rc = kernel_matrix_mul();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_matrix_mul Regression Test (F1)\n");
	int rc = kernel_matrix_mul();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

