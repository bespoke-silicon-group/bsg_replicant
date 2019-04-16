#include "test_rom.h"

int print_rom(uint8_t fd, int idx, int num) {
	hb_mc_response_packet_t buf[num];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 0, idx, num);
	if (read == HB_MC_SUCCESS) {
		for (int i=0; i<num; i++) {
			uint32_t data = hb_mc_response_packet_get_data(&buf[i]);
			bsg_pr_test_info("Read rom data from address 0x%x: 0x%x\n", idx + i, data); 
		}
		return HB_MC_SUCCESS;
	}
	else {
		bsg_pr_test_info("Read from ROM failed.\n");
		return HB_MC_FAIL;
	}
}

int test_rom () {
	uint8_t fd = 0;
	hb_mc_init_host(&fd);

	bsg_pr_test_info("Readback manycore link monitor register\n");
	uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
	bsg_pr_test_info("Recv vacancy is: %x\n", recv_vacancy);
	bsg_pr_test_info("Readback ROM from tile (%d, %d)\n", 0, 0);
	return print_rom(fd, 0, 12);
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_rom Regression Test (COSIMULATION)\n");
	int rc = test_rom();
	*exit_code = rc;
	bsg_pr_test_status(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_rom Regression Test (F1)\n");
	int rc = test_rom();
	bsg_pr_test_status(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

