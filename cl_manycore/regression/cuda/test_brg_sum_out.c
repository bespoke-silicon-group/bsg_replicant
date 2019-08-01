#include "test_brg_sum_out.h"

#define TEST_NAME "test_brg_sum_out"
#define ALLOC_NAME "default_allocator"


void float_sum(float *src, float *result, int M, int N, int P)
{
  //src[M][N][P]  
  float sum = 0;
  for (int i = 0; i < M; i++){
    for (int j = 0; j < P; j++){
      sum = 0;
      for (int k = 0; k < N; k++){
       // printf(" %f", src[k+j*N+i*N*P]);
        sum += src[k*P + j + i*N*P];
      }
      result[j+i*P] = sum;
    }
  }
}

/**
 * Test based on sum input[256, 1069, 800]; dim = 1; out[256,800];
 */
int kernel_brg_sum_out (int M, int N, int P, int t_x, int t_y) {
  int err;
  int size_in = M*N*P;
  int size_out = M*P;
  float *src = malloc(size_in*sizeof(float));
  float *dest = malloc(size_out*sizeof(float));
  float *expected = malloc(size_out*sizeof(float));
  srand(time(0));
  for (int i = 0; i < size_in; i++) {
    src[i] = (float) (rand()/(RAND_MAX/5))
    // src[i] = (float)rand()/(float)(RAND_MAX/1) 
    * (rand()>= (RAND_MAX/2) ? -1 : 1);  
  }
  
  hb_mc_device_t device;
  err = hb_mc_device_init(&device, TEST_NAME, 0);
  if (err) return err;
  char x_str[3];
  char y_str[3];
  snprintf (x_str, sizeof(x_str), "%d", t_x);
  snprintf (y_str, sizeof(y_str), "%d", t_y);
  char* str = BSG_STRINGIFY(BSG_MANYCORE_DIR) 
  "/software/spmd/bsg_cuda_lite_runtime" "/brg_sum_out_";
  char elf[1024];
  strcpy(elf,str);
  strcat(elf,x_str);
  strcat(elf,"x");
  strcat(elf,y_str);
  strcat(elf,"/main.riscv");
  // char* elf = BSG_STRINGIFY(BSG_MANYCORE_DIR) 
  // "/software/spmd/bsg_cuda_lite_runtime" "/brg_sum_out/main.riscv";
  // x_str"x"y_str"/main.riscv";
  bsg_pr_test_info("RISCV: %s\n", elf);
	err = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
  if (err) return err;
  eva_t src_addr, dest_addr;
  err  = hb_mc_device_malloc(&device, size_in*sizeof(float), &src_addr);
  err |= hb_mc_device_malloc(&device, size_out*sizeof(float), &dest_addr);
  if (err) {
    bsg_pr_err(stderr, "hb_mc_device_malloc failed\n");
    return err;
  }
  err  = hb_mc_device_memcpy(&device, (void*)((intptr_t)src_addr),
      &src[0], size_in * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  err  |= hb_mc_device_memcpy(&device, (void*)((intptr_t)dest_addr),
      &dest[0], size_out * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  if (err) {
    bsg_pr_err(stderr, "hb_mc_device_memcpy to device failed\n");
    return err;
  }  
  int block;
  
  int total = t_x*t_y;
  int g_x = 1;
  int g_y = 1;
  int block_M = (M % total == 0) ? M/total : M; //ideally multiple of 16 since total tiles is 16
  int block_P = (P > 100) ? P/100 : P;
  int block_N = 4;
  int dim = 1;
  uint32_t args[] = {src_addr, dest_addr, M, N, P, block_M, block_P, block_N, dim};
  hb_mc_dimension_t grid_dim = {.x = g_x, .y = g_y};
  hb_mc_dimension_t tg_dim = {.x = t_x, .y = t_y};
  err = hb_mc_application_init(&device, grid_dim, tg_dim, "kernel_sum_out", 9, args);
  if (err) return err;
  // Run the kernels 
  float_sum(src, expected, M, N, P); // host 
  err = hb_mc_device_tile_groups_execute(&device); // Manycore
  if (err) return err;
  err = hb_mc_device_memcpy(&device, dest, (void*)((intptr_t)dest_addr),
     size_out * sizeof(float), HB_MC_MEMCPY_TO_HOST);
  if (err) {
      bsg_pr_err(stderr, "hb_mc_device_memcpy to host failed\n");
      return err;
  }
  err = hb_mc_device_finish(&device);
  if (err) return err;
  // Functionality checking
  int mismatch = 0; 
	for (int y = 0; y < M; y ++) { 
		for (int x = 0; x < P; x ++) { 
			if (expected[y * P + x] - dest[y * P + x] > 0.0001 
      || dest[y * P + x] - expected[y * P + x] > 0.0001) {
				printf("Mismatch: C[%d][%d] = % 6.3f\t Expected: % 6.3f.\n", y, x, 
        dest[y * P + x], expected[y * P + x]); 
				mismatch = 1;
			}
		}
	}
  free(src);
  free(dest);
  free(expected);
  if (mismatch) {
    bsg_pr_err(BSG_RED("Matrix Mismatch.\n"));
    return HB_MC_FAIL;
  }
  else bsg_pr_test_info(BSG_GREEN("Matrix Match.\n"));
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
  
  int M = 16;
  int N = 4;
  int P = 2;
  int TILES_X = 4;
  int TILES_Y = 4;
  if (argc > 1) M = atoi(argv[1]);
  if (argc > 2) N = atoi(argv[2]);
  if (argc > 3) P = atoi(argv[3]);
  if (argc > 4) TILES_X = atoi(argv[4]);
  if (argc > 5) TILES_Y = atoi(argv[5]);
  if (M==0) M = 16;

#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("kernel_brg_sum_out Regression Test (COSIMULATION)\n");
	bsg_pr_test_info("argc=%d\n",argc);
	bsg_pr_test_info("Running in[%d][%d][%d] for (%d,%d) tiles\n", M,N,P,TILES_X,TILES_Y);
	int rc = 0;
  if (argc > 5)
    rc = kernel_brg_sum_out(M,N,P,TILES_X,TILES_Y);
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info("kernel_brg_sum_out Regression Test (F1)\n");
	int rc = kernel_brg_sum_out();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
