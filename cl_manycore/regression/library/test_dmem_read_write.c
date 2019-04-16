#include "test_dmem_read_write.h"

int test_dmem_read_write() {
	uint8_t fd; 
	hb_mc_init_host(&fd);	
	
	/* store data in tile */
	uint32_t data = 0xABCD;
	printf("BSG INFO: Writing to DMEM\n");
	int write = hb_mc_copy_to_epa(0, 0, 1, DMEM_BASE >> 2, &data, 1);
	
	if (write != HB_MC_SUCCESS) {
		printf("BSG INFO: Writing data to tile (0, 0)'s DMEM failed.\n");
		return HB_MC_FAIL;
	}
	printf("BSG INFO: Write successful\n");
	/* read back data */
	hb_mc_response_packet_t buf[1];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
	printf("BSG INFO: Completed read\n");
	if (read == HB_MC_SUCCESS) {
		uint32_t rdata = hb_mc_response_packet_get_data(&buf[0]);
		if (rdata == data) {
			printf("BSG INFO: Read packet data: 0x%x\n", hb_mc_response_packet_get_data(&buf[0]));
			return HB_MC_SUCCESS;
		}
		else {
			printf("BSG INFO: Packet data mismatch!\n");
			return HB_MC_FAIL;
		}	
	}	
	else {
		printf("BSG INFO: read from tile failed.\n");
		return HB_MC_FAIL;
	}
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	printf("BSG INFO: test_dmem_read_write Regression Test (COSIMULATION)\n");
	int rc = test_dmem_read_write();
	*exit_code = rc;
	if (rc == HB_MC_SUCCESS)
		printf("BSG REGRESSION TEST PASSED\n");
	else
		printf("BSG REGRESSION TEST FAILED\n");
	return;
}
#else
int main() {
	printf("BSG INFO: test_dmem_read_write Regression Test (F1)\n");
	int rc = test_dmem_read_write();
	if (rc == HB_MC_SUCCESS)
		printf("BSG REGRESSION TEST PASSED\n");
	else
		printf("BSG REGERSSION TEST FAILED\n");
	return rc;
}
#endif
