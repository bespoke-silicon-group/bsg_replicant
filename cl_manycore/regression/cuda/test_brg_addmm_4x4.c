#include "test_brg_addmm.h"

#define TEST_NAME "test_brg_addmm"
#define ALLOC_NAME "default_allocator"

/*! 
 * Matrix multiplication code on the host side to compare the results
 */
void matrix_mult (float *A, float *B, float *C, float a, float b, 
int M, int N, int P) { 
	for (int y = 0; y < M; y ++) { 
		for (int x = 0; x < P; x ++) { 
			int res = 0;
			for (int k = 0; k < N; k++) { 
				res += A[y * N + k] * B[k * P + x];
			}
			C[y * P + x] = res;
		}
	}
	return;
}

/**
 * Adds two same size matrices together
 */
void matrix_add(float *mat1, float *mat2, float* mat_out, int M, int N){
  for(int i = 0; i < M; i++){
    for(int j = 0; j < N; j++){
      mat_out[i*N+j] = mat1[i*N+j] + mat2[i*N+j]; 
    }
  }
}		


int kernel_matrix_mul (int M, int N, int P, int t_x, int t_y) {
  int rc;
	srand(time); 
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
  char x_str[3];
  char y_str[3];
  snprintf (x_str, sizeof(x_str), "%d", t_x);
  snprintf (y_str, sizeof(y_str), "%d", t_y);
  char* str = BSG_STRINGIFY(BSG_MANYCORE_DIR) 
  "/software/spmd/bsg_cuda_lite_runtime" "/brg_addmm_";
  char elf[1024];
  strcpy(elf,str);
  strcat(elf,x_str);
  strcat(elf,"x");
  strcat(elf,y_str);
  strcat(elf,"/main.riscv");

	// char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) 
  // "/software/spmd/bsg_cuda_lite_runtime" "/brg_addmm/main.riscv";
	rc = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
	if (rc != HB_MC_SUCCESS) { 
		bsg_pr_err("failed to initialize program.\n");
		return rc;
	}


	/*****************************************************************************************************************
	* Allocate memory on the device for A, B and C.
	******************************************************************************************************************/
	// uint32_t M = 4;
	// uint32_t N = 8;
	// uint32_t P = 4;
  float* mat = malloc(M*P*sizeof(float));
  float* mat1 = malloc(M*N*sizeof(float));
  float* mat2 = malloc(N*P*sizeof(float));
  float* mat_out = malloc(M*P*sizeof(float));
  float* exp_result = malloc(M*P*sizeof(float));
  float a = 1;
  float b = 1; 
  eva_t mat_dev, mat1_dev, mat2_dev, mat_out_dev; 
  rc  = hb_mc_device_malloc(&device, M * P * sizeof(float), &mat_dev);
  rc |= hb_mc_device_malloc(&device, M * N * sizeof(float), &mat1_dev);
  rc |= hb_mc_device_malloc(&device, N * P * sizeof(float), &mat2_dev);
  rc |= hb_mc_device_malloc(&device, M * P * sizeof(float), &mat_out_dev);
  if (rc) {
      bsg_pr_err("hb_mc_device_malloc failed\n");
      return rc;
  }
  /*****************************************************************************************************************
	* Allocate memory on the host for A & B and initialize with random values.
	******************************************************************************************************************/
	for (int i = 0; i < M * P; i++) { /* fill mat with arbitrary data */
		mat[i] = (float)rand()/(float)(RAND_MAX/1) * 
    ((rand() > RAND_MAX/2) ? -1 : 1);
	}
  for (int i = 0; i < M * N; i++) { /* fill mat1 with arbitrary data */
    mat1[i] = (float)(rand()/(RAND_MAX/10)) * //(float)rand()/(float)(RAND_MAX/1) * 
    ((rand() > RAND_MAX/2) ? -1 : 1);
	}
  for (int i = 0; i < N * P; i++) { /* fill mat2 with arbitrary data */
		mat2[i] = (float)(rand()/(RAND_MAX/10)) * //(float)rand()/(float)(RAND_MAX/1) * 
    ((rand() > RAND_MAX/2) ? -1 : 1);
	}
  matrix_mult (mat1, mat2, exp_result, a, b, M, N, P);
  matrix_add(mat,exp_result,exp_result, M, P);


	/*****************************************************************************************************************
	* Copy A & B from host onto device DRAM.
	******************************************************************************************************************/
	rc  = hb_mc_device_memcpy(&device, (void*)((float*)mat_dev),
        mat, M * P * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  rc |= hb_mc_device_memcpy(&device, (void*)((float*)mat1_dev),
      mat1, M * N * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  rc |= hb_mc_device_memcpy(&device, (void*)((float*)mat2_dev),
      mat2, N * P * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  rc |= hb_mc_device_memcpy(&device, (void*)((float*)mat_out_dev),
      mat_out, M * P * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  if (rc) {
      bsg_pr_err("hb_mc_device_memcpy to device failed\n");
      return rc;
  }


	/*****************************************************************************************************************
	* Define block_size_x/y: amount of work for each tile group
	* Define tg_dim_x/y: number of tiles in each tile group
	* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
	******************************************************************************************************************/
	uint32_t block_size_x = 4;
	uint32_t block_size_y = 4;
  int grid_x = (P/block_size_x > 0) ? P/block_size_x : 1;  
  int grid_y = (M/block_size_y > 0) ? M/block_size_y : 1;  
	hb_mc_dimension_t tg_dim = { .x = t_x, .y = t_y };

	hb_mc_dimension_t grid_dim = { .x = grid_x, .y = grid_y };


	/*****************************************************************************************************************
	* Prepare list of input arguments for kernel.
	******************************************************************************************************************/
	int argv[] = {mat_dev, mat1_dev, mat2_dev, mat_out_dev, 
  a, b, M, N, P, block_size_y, block_size_x};
	/*****************************************************************************************************************
	* Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
	******************************************************************************************************************/
	rc = hb_mc_application_init (&device, grid_dim, tg_dim, 
  "kernel_brg_addmm", 11, argv);
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
	rc = hb_mc_device_memcpy(&device, mat_out, 
  (void*)((float*)mat_out_dev), M * P * sizeof(float), 
  HB_MC_MEMCPY_TO_HOST);
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
	/*****************************************************************************************************************
	* Calculate the expected result matrix using host code and compare the results. 
	******************************************************************************************************************/
  int mismatch = 0;
  for (int y = 0; y < M; y ++) { 
		for (int x = 0; x < P; x ++) { 
			if (exp_result[y * P + x] != mat_out[y * P + x]) {
				bsg_pr_err(BSG_RED("Mismatch: ") 
        "C[%d][%d] = % 8.6f\t Expected: % 8.6f.\n",
         y, x, mat_out[y * P + x], exp_result[y * P + x]); 
				mismatch = 1;			
      }
		}
	}
	if (mismatch) { 
		bsg_pr_err(BSG_RED("Matrix Mismatch.\n"));
		return HB_MC_FAIL;
	}
	bsg_pr_test_info(BSG_GREEN("Matrix Match.\n"));
	free(mat);
  free(mat1);
  free(mat2);
  free(exp_result);
  free(mat_out);
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
  
  int M = 4;
  int N = 4;
  int P = 4;
  int TILES_X = 4;
  int TILES_Y = 4;
  if (argc > 1) M = atoi(argv[1]);
  if (argc > 2) N = atoi(argv[2]);
  if (argc > 3) P = atoi(argv[3]);
  if (argc > 4) TILES_X = atoi(argv[4]);
  if (argc > 5) TILES_Y = atoi(argv[5]);
  if (M==0) M = 4;
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_brg_addmm Regression Test (COSIMULATION)\n");
	bsg_pr_test_info("argc=%d\n",argc);
	bsg_pr_test_info("Running in[%d][%d][%d] for (%d,%d) tiles\n", M,N,P,TILES_X,TILES_Y);
	int rc = 0;
  // if (argc > 5)
    rc = kernel_matrix_mul(M,N,P,TILES_X,TILES_Y);
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info("test_brg_addmm Regression Test (F1)\n");
	int rc = kernel_matrix_mul(M,N,P,TILES_X,TILES_Y);
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
