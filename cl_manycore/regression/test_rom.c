#include "manycore_tests.h"
#include "test_rom.h"

int print_rom(uint8_t fd, int idx, int num) {
	hb_mc_response_packet_t buf[num];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 0, idx, num);
	if (read == HB_MC_SUCCESS) {
		for (int i=0; i<num; i++) {
			uint32_t data = hb_mc_response_packet_get_data(&buf[i]);
			printf("read rom data @ address %d: 0x%x\n", idx + i, data); 
	}
		return HB_MC_SUCCESS;
	}
	else {
		printf("read from ROM failed.\n");
		return HB_MC_FAIL;
	}
}


int test_rom () {
	printf("Runing the Bladerunner ROM test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd = 0;
	hb_mc_init_host(&fd);

	printf("Readback manycore link monitor register\n");
	uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
	printf("recv vacancy is: %x\n", recv_vacancy);
	printf("Readback ROM from tile (%d, %d)\n", 0, 0);
	return print_rom(fd, 0, 12);
}

#ifdef COSIM
	void test_main(uint32_t *exit_code) {	
		printf("Regression Test on COSIMULATION:\n\n");
		int rc = test_rom();
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
		int rc = test_rom();
		if (rc == HB_MC_SUCCESS)
			printf("TEST PASSED~~~\n");
		else
			printf("TEST FAILED!!!\n");
		return rc;
	}
#endif

