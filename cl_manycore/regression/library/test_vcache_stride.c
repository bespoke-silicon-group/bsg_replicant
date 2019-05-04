#include "test_vcache_stride.h"

#define STRIDE_SIZE 1024
#define DRAM_BASE 0x0000
#define ARRAY_LEN 16

int test_vcache_stride() {
 
	srand(time(0));
	uint8_t fd; 
	if (hb_mc_fifo_init(&fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "Failed to initialize host.\n");
		return HB_MC_FAIL;
	}

	uint32_t host_coord_x, host_coord_y, manycore_dim_x, manycore_dim_y;
	if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &host_coord_x) == HB_MC_FAIL) {
                fprintf(stderr, "Failed to read host X coordinate from rom.\n");
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &host_coord_y) == HB_MC_FAIL) {
                fprintf(stderr, "Failed to read host Y coordinate from rom.\n");
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_X, &manycore_dim_x) == HB_MC_FAIL) {
                fprintf(stderr, "Failed to read device X dimension from rom.\n");
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_Y, &manycore_dim_y) == HB_MC_FAIL) {
                fprintf(stderr, "Failed to read device Y dimension from rom.\n");
                return HB_MC_FAIL;
        }
        

	uint8_t dram_coord_x = 0;
	uint8_t dram_coord_y = manycore_dim_y + 1;


	/* For now only tests the first DRAM bank. To test all, change the loop to dram_coord_x < manycore_dim_x */
	for (dram_coord_x = 0 ; dram_coord_x < 1; dram_coord_x ++) {

		fprintf(stderr, "Testing DRAM bank (%d,%d).\n", dram_coord_x, dram_coord_y);

		uint32_t A_host [ARRAY_LEN];
		uint32_t A_device [ARRAY_LEN]; 

		for (int i = 0; i < ARRAY_LEN; i ++) { 
			A_host[i] = rand();
		}


		for (int stride = 0; stride < ARRAY_LEN; stride ++) {
			uint32_t dram_addr = DRAM_BASE + stride * STRIDE_SIZE;

	        	hb_mc_request_packet_t req;
		        hb_mc_response_packet_t res;
        
        		hb_mc_format_request_packet(&req, dram_addr, A_host[stride], dram_coord_x, dram_coord_y, HB_MC_PACKET_OP_REMOTE_STORE);
       
	        	if(hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, (hb_mc_packet_t *)(&req)) != HB_MC_SUCCESS) {
        	        	fprintf(stderr, "Failed to write to FIFO\n");
	        	        return HB_MC_FAIL;
	        	}


		        hb_mc_request_packet_set_op(&req, HB_MC_PACKET_OP_REMOTE_LOAD);
		        if(hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, (hb_mc_packet_t *)(&req)) != HB_MC_SUCCESS) {
        		        fprintf(stderr, "Failed to write to FIFO\n");
		                return HB_MC_FAIL;
        		}
	        	if(hb_mc_fifo_receive(fd, HB_MC_FIFO_RX_RSP, (hb_mc_packet_t *)(&res)) != HB_MC_SUCCESS) {
        	        	fprintf(stderr, "Failed to read from FIFO\n");
	        	        return HB_MC_FAIL;
	        	}

		        A_device[stride] = hb_mc_response_packet_get_data(&res);


			if(A_host[stride] == A_device[stride]) { 
				fprintf(stderr, "Success -- A_host[%d] = %d   ==   A_device[%d] = %d - EPA = %d.\n", stride, A_host[stride], stride, A_device[stride], dram_addr);
			}
			else {
				fprintf(stderr, "Failed -- A_host[%d] = %d   !=   A_device[%d] = %d - EPA = %d.\n", stride, A_host[stride], stride, A_device[stride], dram_addr);
				return HB_MC_FAIL;
			}
		}
	}
	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_vcache_stride Regression Test (COSIMULATION)\n");
	int rc = test_vcache_stride();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_vcache_stride Regression Test (F1)\n");
	int rc = test_vcache_stride();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
