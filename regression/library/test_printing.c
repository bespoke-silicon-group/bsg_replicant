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
void cosim_main(uint32_t *exit_code, char * args) {
	// We aren't passed command line arguments directly so we parse them
	// from *args. args is a string from VCS - to pass a string of arguments
	// to args, pass c_args to VCS as follows: +c_args="<space separated
	// list of args>"
	int argc = get_argc(args);
	char *argv[argc];
	get_argv(args, argc, argv);

#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_printing Regression Test (COSIMULATION)\n");
	int rc = test_printing();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info("test_printing Regression Test (F1)\n");
	int rc = test_printing();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
