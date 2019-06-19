#include "test_unified_main.h"

#ifdef COSIM
void test_main(uint32_t *exit_code) {
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
        bsg_pr_test_info("test_unified_main Regression Test (COSIMULATION)\n");
        int rc = HB_MC_SUCCESS;
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main() {
        bsg_pr_test_info("test_unified_main Regression Test (F1)\n");
        int rc = HB_MC_SUCCESS;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif

