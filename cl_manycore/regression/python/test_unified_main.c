#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "python_tests.h"

int test_python(int argc, char **argv) {
        int len;
        const char *python_path = BSG_STRINGIFY(BSG_PYTHON_TEST_PATH) "/";
        char *test_name;

        struct arguments args = {NULL};

        argp_parse (&argp_name, argc, argv, 0, 0, &args);
        test_name = args.testname;
        bsg_pr_test_info("%s Regression Test\n", test_name);

        len = strlen(python_path) + strlen(test_name) + strlen(".py");

        char program_path[len + 1], *ptr;
        program_path[len] = '\0';
        ptr = program_path;

        strcpy(ptr, python_path);
        ptr += strlen(python_path);

        strcpy(ptr, test_name);
        ptr += strlen(test_name);

        strcpy(ptr, ".py");
        ptr += strlen(".py");
        *ptr = '\0';

        Py_Initialize();

        PyObject *obj = Py_BuildValue("s", program_path);
        FILE *fp = _Py_fopen_obj(obj, "r+");

        PyRun_SimpleFileEx(fp, program_path, 0);

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
        int rc = test_python(argc, argv);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char **argv) {
        bsg_pr_test_info("test_python Regression Test (F1)\n");
        int rc = test_python(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif
