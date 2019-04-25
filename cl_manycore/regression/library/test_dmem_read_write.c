#include "test_dmem_read_write.h"

int test_dmem_read_write() {
	uint8_t fd; 
	hb_mc_init_host(&fd);	

	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "test_dmem_read_write(): failed to initialize host.\n");
		return HB_MC_FAIL;
	}
		
	/* store data in tile */
	uint32_t data = 0xABCD;
	bsg_pr_test_info("Writing to DMEM\n");
	int write = hb_mc_copy_to_epa(0, 0, 1, DMEM_BASE >> 2, &data, 1);
	
	if (write != HB_MC_SUCCESS) {
		bsg_pr_test_info("Writing data to tile (0, 0)'s DMEM failed.\n");
		return HB_MC_FAIL;
	}
	bsg_pr_test_info("Write successful\n");

	/* read back data */
	hb_mc_response_packet_t buf[1];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
	bsg_pr_test_info("Completed read\n");

	if (hb_mc_host_finish(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "test_dmem_read_write(): failed to terminate host.\n");
		return HB_MC_FAIL;
	}

	if (read == HB_MC_SUCCESS) {
		uint32_t rdata = hb_mc_response_packet_get_data(&buf[0]);
		if (rdata == data) {
			bsg_pr_test_info("Read packet data: 0x%x\n", hb_mc_response_packet_get_data(&buf[0]));
			return HB_MC_SUCCESS;
		}
		else {
			bsg_pr_test_info("Packet data mismatch!\n");
			return HB_MC_FAIL;
		}	
	}	
	else {
		bsg_pr_test_info("read from tile failed.\n");
		return HB_MC_FAIL;
	}
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_dmem_read_write Regression Test (COSIMULATION)\n");
	int rc = test_dmem_read_write();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_dmem_read_write Regression Test (F1)\n");
	int rc = test_dmem_read_write();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
