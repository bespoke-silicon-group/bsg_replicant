#include "test_brg_embedding_backward.h"

#define TEST_NAME "test_brg_embedding_backward"
#define ALLOC_NAME "default_allocator"

void host_brg_embedding_backward(int *idata, float *gdata, float *wdata, int M, int N, int P)
{
  for(int i = 0; i < N; i++){
    int k = idata[i];
    for( int l = 0; l < P; l++){
      wdata[P*k+l] += gdata[P*i+l];
    }
  }
}

/**
 * Runs the embedding backward kernel
 */
int kernel_brg_embedding_backward (int M, int N, int P, int t_x, int t_y) {
  int32_t *idata = malloc(N * sizeof(int32_t));
  float *gdata = malloc(N * P * sizeof(float));
  float *wdata = malloc(M * P * sizeof(float));
  float *wdata_exp = malloc(M * P * sizeof(float));
  for (int i = 0; i < P * M; i++) {
    wdata[i] = (float)rand()/(float)(RAND_MAX/1) 
    * (rand()>= (RAND_MAX/2) ? -1 : 1); 
    wdata_exp[i] = wdata[i];
  }
  for (int i = 0; i < P * N; i++){
    gdata[i] = (float)rand()/(float)(RAND_MAX/1) 
    * (rand()>= (RAND_MAX/2) ? -1 : 1);  
  }
  for (int j = 0; j < N; j++){
    idata[j] = rand() % M;
  }
  int err;
  hb_mc_device_t device;
  err = hb_mc_device_init(&device, TEST_NAME, 0);
  if (err) return err;
  char x_str[3];
  char y_str[3];
  snprintf (x_str, sizeof(x_str), "%d", t_x);
  snprintf (y_str, sizeof(y_str), "%d", t_y);
  char* str = BSG_STRINGIFY(BSG_MANYCORE_DIR) 
  "/software/spmd/bsg_cuda_lite_runtime" "/brg_embedding_backward_";
  char elf[1024];
  strcpy(elf,str);
  strcat(elf,x_str);
  strcat(elf,"x");
  strcat(elf,y_str);
  strcat(elf,"/main.riscv");
  bsg_pr_test_info("RISCV: %s\n", elf);
  eva_t idata_dev_addr, gdata_dev_addr, wdata_dev_addr;
  err = hb_mc_device_program_init(&device, elf, ALLOC_NAME, 0);
  if (err) return err;
  err  = hb_mc_device_malloc(&device, N*sizeof(int), &idata_dev_addr);
  err |= hb_mc_device_malloc(&device, M*P*sizeof(float), &wdata_dev_addr);
  err |= hb_mc_device_malloc(&device, N*P*sizeof(float), &gdata_dev_addr);
  if (err) {
    fprintf(stderr, "hb_mc_device_malloc failed\n");
    return err;
  }
  err  = hb_mc_device_memcpy(&device, (void*)((intptr_t)idata_dev_addr),
      &idata[0], N * sizeof(int), HB_MC_MEMCPY_TO_DEVICE);
  err  |= hb_mc_device_memcpy(&device, (void*)((intptr_t)wdata_dev_addr),
      &wdata[0], M*P * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  err  |= hb_mc_device_memcpy(&device, (void*)((intptr_t)gdata_dev_addr),
      &gdata[0], N*P * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
  if (err) {
    fprintf(stderr, "hb_mc_device_memcpy to device failed\n");
    return err;
  }
  int total = t_x*t_y;
  int block_M = (M%total==0 && M>=total) ? M/total : M;
  uint32_t args[] = {idata_dev_addr, gdata_dev_addr, wdata_dev_addr, M, N, P, block_M};
  int grid_x = 1;  
  int grid_y = 1; 
  hb_mc_dimension_t grid_dim = {.x = grid_x, .y = grid_y};
  hb_mc_dimension_t tg_dim = {.x = t_x, .y = t_y};
  err = hb_mc_application_init(&device, grid_dim, tg_dim, "kernel_embedding_backward", 7, args);
  if (err) return err;
  err = hb_mc_device_tile_groups_execute(&device);
  if (err) return err;
  err = hb_mc_device_memcpy(&device, wdata, (void*)((intptr_t)wdata_dev_addr),
     M*P * sizeof(float), HB_MC_MEMCPY_TO_HOST);
  if (err) {
      fprintf(stderr, "hb_mc_device_memcpy to host failed\n");
      return err;
  }
  err = hb_mc_device_finish(&device);
  if (err) return err;
  host_brg_embedding_backward(idata, gdata, wdata_exp, M, N, P);
  int mismatch = 0; 
	for (int y = 0; y < M; y ++) { 
		for (int x = 0; x < P; x ++) { 
			if (wdata_exp[y * P + x] - wdata[y * P + x] > 0.0001 
      || wdata[y * P + x] - wdata_exp[y * P + x] > 0.0001) {
				printf("Mismatch: C[%d][%d] = % 6.3f\t Expected: % 6.3f.\n", y, x, 
        wdata[y * P + x], wdata_exp[y * P + x]); 
				mismatch = 1;
			}
		}
	}
  free(gdata);
  free(wdata);
  free(wdata_exp);
  free(idata);
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
  int N = 5;
  int P = 5;
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
	bsg_pr_test_info("test_brg_embedding_backward Regression Test (COSIMULATION)\n");
	bsg_pr_test_info("idx[%d], grad[%d][%d], grad_w[%d][%d] for (%d,%d) tiles\n", 
  N,N,P,M,P,TILES_X,TILES_Y);
	int rc = 0;
  // if (argc > 5)
    rc = kernel_brg_embedding_backward(M,N,P,TILES_X,TILES_Y);
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info("test_brg_embedding_backward Regression Test (F1)\n");
	int rc = kernel_brg_embedding_backward();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif