#include "test_struct_size.h"

int test_struct_size() {
	size_t sz = sizeof(hb_mc_packet_t);
	if(sz == 16){
		return HB_MC_SUCCESS;
	} else {
		bsg_pr_test_info("Unexpected size of hb_mc_packet_t (%lu). Fail.\n", sz);
		return HB_MC_FAIL;
	}
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_struct_size Regression Test (COSIMULATION)\n");
	int rc = test_struct_size();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_struct_size Regression Test (F1)\n");
	int rc = test_struct_size();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
