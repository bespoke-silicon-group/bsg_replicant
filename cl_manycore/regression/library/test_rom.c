#include "test_rom.h"

int read_npa_rom(uint8_t fd, int idx, int num, /* out */ uint32_t *result) {
	hb_mc_response_packet_t buf[num];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 0, idx, num);
	if (read == HB_MC_SUCCESS) {
		for (int i=0; i<num; i++) {
			result[i] = hb_mc_response_packet_get_data(&buf[i]);
			bsg_pr_test_info("Read rom data from address 0x%x: 0x%x\n", idx + i, result[i]); 
		}
		return HB_MC_SUCCESS;
	}
	bsg_pr_test_info("Read from ROM failed.\n");
	return HB_MC_FAIL;
}

int read_rom(uint8_t fd, int num, /* out */ uint32_t *result) {
	int rc;
	rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_X, &result[0]);
	if(rc == HB_MC_FAIL)
		return rc;
	rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_Y, &result[1]);
	if(rc == HB_MC_FAIL)
		return rc;
	rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &result[2]);
	if(rc == HB_MC_FAIL)
		return rc;
	rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &result[3]);
	if(rc == HB_MC_FAIL)
		return rc;
	return HB_MC_SUCCESS;
}

int test_rom () {
	int rc = 0;
	uint8_t fd = 0;

	if(hb_mc_fifo_init(&fd) != HB_MC_SUCCESS){
		fprintf(stderr, "test_rom(): failed to initialize host.\n");
		return HB_MC_FAIL;
	}

	// In order, at offset 4, the elements of the array are:
	// NETWORK_DIM_X, NETWORK_DIM_Y, HOST_INTERFACE_COORD_X, HOST_INTERFACE_COORD_Y
	const char *desc[4] = { "Network X Dimension", "Network Y Dimension", "Host X Coordinate", "Host Y Coordinate" };
	uint32_t expected[4] = { CL_MANYCORE_DIM_X, CL_MANYCORE_DIM_Y, 0, 0 };
	uint32_t result[4] = {};


	read_rom(fd, 4, result); 
	bsg_pr_test_info("Comparing AXI space results:\n");
	fflush(stderr);
	rc = compare_results(4, desc, expected, result);
	if(rc != HB_MC_SUCCESS){
		return HB_MC_FAIL;
	}
 
	bsg_pr_test_info("Readback manycore link monitor register\n");
	uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
	uint32_t host_credits = hb_mc_get_host_credits(fd);
	bsg_pr_test_info("Recv vacancy is: %x\n", recv_vacancy);
	bsg_pr_test_info("host credits is: %x\n", host_credits);

//	if(read_npa_rom(fd, 4, 4, result) == HB_MC_FAIL) {
//		return HB_MC_FAIL;
//	}
//	bsg_pr_test_info("Comparing NPA space results:\n");
//	rc = compare_results(4, desc, expected, result);
//	if(rc != HB_MC_SUCCESS){
//		return HB_MC_FAIL;
//	}

	if(hb_mc_fifo_finish(fd) != HB_MC_SUCCESS){
		fprintf(stderr, "test_rom(): failed to terminate host.\n");
		return HB_MC_FAIL;
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

