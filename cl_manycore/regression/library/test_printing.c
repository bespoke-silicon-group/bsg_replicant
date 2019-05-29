/********************************************************************/
/* This tests the print macros defined in `bsg_manycore_printing.h` */
/* The expected output is:                                          */
/*                                                                  */
/* DEBUG:   testing 1... 2... 3...                                  */
/* DEBUG:   hello                                                   */
/* DEBUG:   from                                                    */
/* DEBUG:   test_printing                                           */
/* DEBUG:                                                           */
/* DEBUG:   done                                                    */
/* ERROR:   testing error                                           */
/* WARNING: 1                                                       */
/* WARNING: 2                                                       */
/* WARNING: hello                                                   */
/* INFO:    hello from                                              */
/* INFO:    info                                                    */
/********************************************************************/

#define DEBUG
#include "test_printing.h"
static int test_printing(void)
{
        bsg_pr_dbg("testing ");
        bsg_pr_dbg("1... ");
        bsg_pr_dbg("2... ");
        bsg_pr_dbg("3... ");
        bsg_pr_dbg("\n");
        
        bsg_pr_dbg("hello\nfrom\n%s\n", __func__);
        bsg_pr_dbg("\ndone\n");

        bsg_pr_err("testing error");
        bsg_pr_err("\n");

        bsg_pr_warn("%d\n%d\n%s\n", 1, 2, "hello");

        bsg_pr_info("%s %s\n%s\n", "hello", "from", "info");
        return 0;
}
#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_printing Regression Test (COSIMULATION)\n");
	int rc = test_printing();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_printing Regression Test (F1)\n");
	int rc = test_printing();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
