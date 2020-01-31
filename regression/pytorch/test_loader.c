// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "pytorch_tests.h"

int test_pytorch(int argc, char **argv) {
        int len, err;
        char *pytorch_path;
        char *test_name;

        struct arguments_path args = {NULL};

        argp_parse (&argp_path_py, argc, argv, 0, 0, &args);
        test_name = args.name;
        pytorch_path = args.path;
        bsg_pr_test_info("%s Regression Test\n", test_name);

        len = strlen(pytorch_path) + strlen(test_name) + strlen(".py");

        char program_path[len + 1], *ptr;
        program_path[len] = '\0';
        ptr = program_path;

        strcpy(ptr, pytorch_path);
        ptr += strlen(pytorch_path);

        strcpy(ptr, test_name);
        ptr += strlen(test_name);

        strcpy(ptr, ".py");
        ptr += strlen(".py");
        *ptr = '\0';

        Py_Initialize();

        PyObject *obj = Py_BuildValue("s", program_path);
        FILE *fp = _Py_fopen_obj(obj, "r+");

        err = PyRun_SimpleFileEx(fp, program_path, 0);

        if (Py_FinalizeEx() < 0) {
                exit(120);
        }
        fclose(fp);

        return err;
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
        bsg_pr_test_info("test_pytorch Regression Test (COSIMULATION)\n");
        int rc = test_pytorch(argc, argv);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char **argv) {
        bsg_pr_test_info("test_pytorch Regression Test (F1)\n");
        int rc = test_pytorch(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif
