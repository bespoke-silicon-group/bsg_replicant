#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "python_tests.h"

int test_python() {
	Py_Initialize();
	const char* test_name =
		BSG_STRINGIFY(BSG_PYTHON_TEST_PATH) "/"
		BSG_STRINGIFY(BSG_TEST_NAME)".py";
	PyObject *obj = Py_BuildValue("s", test_name);
	FILE *fp = _Py_fopen_obj(obj, "r+");

	PyRun_SimpleFileEx(fp, test_name, 0);

	if (Py_FinalizeEx() < 0) {
		exit(120);
	}
	fclose(fp);

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
	bsg_pr_test_info("test_python Regression Test (COSIMULATION)\n");
	int rc = test_python();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info("test_python Regression Test (F1)\n");
	int rc = test_python();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
