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

int read_rom(uint8_t fd, int idx, int num, /* out */ uint32_t *result) {
    	hb_mc_response_packet_t buf[num];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 0, idx, num);
	if (read == HB_MC_SUCCESS) {
		for (int i=0; i<num; i++) {
			result[i] = hb_mc_response_packet_get_data(&buf[i]);
			bsg_pr_test_info("Read rom data from address 0x%x: 0x%x\n", idx + i, result); 
		}
		return HB_MC_SUCCESS;
	}
        bsg_pr_test_info("Read from ROM failed.\n");
        return HB_MC_FAIL;
}

int read_axi_rom(uint8_t fd, int idx) {
	for (int i=0; i<12; i++) {
		printf("0x%x\n", hb_mc_read32(fd, 0x2000 + i));
	}
	printf("0x%x\n", hb_mc_read32(fd, 0x2000 + 0x100));
	printf("0x%x\n", hb_mc_read32(fd, 0x2000 + 0x200));
  return HB_MC_SUCCESS;
}


int test_rom () {
	uint8_t fd = 0;
	hb_mc_init_host(&fd);

	bsg_pr_test_info("Readback manycore link monitor register\n");
	uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
	bsg_pr_test_info("Recv vacancy is: %x\n", recv_vacancy);
	bsg_pr_test_info("Readback ROM from tile (%d, %d)\n", 0, 0);

        // In order, the elements of the array are:
        // NETWORK_DIM_X, NETWORK_DIM_Y, HOST_INTERFACE_COORD_X, HOST_INTERFACE_COORD_Y
        const char *desc[4] = { "Network X Dimension", "Network Y Dimension", "Host X Coordinate", "Host Y Coordinate" };
        uint32_t expected[4] = { CL_MANYCORE_DIM_X, CL_MANYCORE_DIM_Y, 0, CL_MANYCORE_DIM_Y - 1 };
        uint32_t actual[4] = {};
       	read_axi_rom(fd, 0x2000); 
        if(read_rom(fd, 4, 4, actual) == HB_MC_FAIL) {
		return HB_MC_FAIL;
        }
        bsg_pr_test_info("Comparing results:\n");
        for(int i = 0; i < 4; i++) {
		bsg_pr_test_info("%s: Expected = %d, Actual = %d", desc[i], expected[i], actual[i]);
		if(expected[i] != actual[i]) {
			printf("\033[31m Failed \033[0m\n");
			return HB_MC_FAIL;
		}
		printf("\033[033m Succeeded \033[0m\n");
        }
        return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_rom Regression Test (COSIMULATION)\n");
	int rc = test_rom();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_rom Regression Test (F1)\n");
	int rc = test_rom();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

