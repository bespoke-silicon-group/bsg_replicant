#include "test_manycore_compile.h"
static
int test_manycore_compile(void)
{
	return 0;
}
#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_manycore_compile Regression Test (COSIMULATION)\n");
	int rc = test_manycore_compile();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_manycore_compile Regression Test (F1)\n");
	int rc = test_manycore_compile();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
