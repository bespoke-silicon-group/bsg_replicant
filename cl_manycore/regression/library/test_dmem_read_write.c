#include "test_dmem_read_write.h"

int test_dmem_read_write() {
	
	printf("Running the Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");
		
	uint8_t fd; 
	hb_mc_init_host(&fd);	
	
	/* store data in tile */
	uint32_t data = 0xABCD;
	printf("write to DMEM\n");
	int write = hb_mc_copy_to_epa(0, 0, 1, DMEM_BASE >> 2, &data, 1);
	
	if (write != HB_MC_SUCCESS) {
		printf("writing data to tile (0, 0)'s DMEM failed.\n");
		return HB_MC_FAIL;
	}
	printf("write success\n");
	/* read back data */
	hb_mc_response_packet_t buf[1];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
	printf("completed read.\n");
	if (read == HB_MC_SUCCESS) {
		uint32_t rdata = hb_mc_response_packet_get_data(&buf[0]);
		if (rdata == data) {
			printf("read packet data: 0x%x\n", hb_mc_response_packet_get_data(&buf[0]));
			return HB_MC_SUCCESS;
		}
		else {
			printf("packet data mismatch!");
		}	
	}	
	else {
		printf("read from tile failed.\n");
		return HB_MC_FAIL;
	}
}


#ifdef COSIM
	void test_main(uint32_t *exit_code) {	
		printf("Regression Test on COSIMULATION:\n\n");
		int rc = test_dmem_read_write();
		*exit_code = rc;
		if (rc == HB_MC_SUCCESS)
			printf("TEST PASSED~~~\n");
		else
			printf("TEST FAILED!!!\n");
		return;
	}
#else
	int main() {
		printf("Regression Test on F1:\n\n");
		int rc = test_dmem_read_write();
		if (rc == HB_MC_SUCCESS)
			printf("TEST PASSED~~~\n");
		else
			printf("TEST FAILED!!!\n");
		return rc;
	}
#endif
