#ifndef _CL_MANYCORE_REGRESSION_H
#define _CL_MANYCORE_REGRESSION_H

/**
 * bsg_pr_test_info() a version of printf. All regression tests should prefer this function over direct stdio calls.
 */
#define bsg_pr_test_info(fmt, ...)					\
    printf("BSG INFO: " fmt, ##__VA_ARGS__)

/**
 * bsg_pr_test_pass_fail() prints a success/fail message depending on a test condition
 * @param[in] success_condition a condition which, if true, indicates that the test has passed
 */
#define bsg_pr_test_pass_fail(success_condition)			\
    printf("BSG RERGRESSION TEST %s\n", ((success_condition) ? "PASSED" : "FAILED"))

int compare_word(int size, const char *desc[], uint32_t *expected, uint32_t *actual) {
	int rc = 0;
	for(int i = 0; i < size; i++) {
		bsg_pr_test_info("%s: Expected = %d, Actual = %d", desc[i], expected[i], actual[i]);
		if(expected[i] != actual[i]) {
			printf("\033[31m Failed \033[0m\n");
			rc = 1;
		} 
		else
			printf("\033[032m Succeeded \033[0m\n");
	}
	if(rc)
		return HB_MC_FAIL;
	else
		return HB_MC_SUCCESS;
}

#endif
