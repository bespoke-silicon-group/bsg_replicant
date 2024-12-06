#include <stdint.h>
#include <unistd.h>
#include <bsg_manycore_regression.h>
#include <dlfcn.h>

// This function is main for Verilator cosimulation
int main(int argc, char **argv) {

        // To reduce the number of Verilator compilations we do, this
        // platform compiles the executable machine and then loads the
        // program as a shared object file. This shared object is
        // passed as the string sopath and _must_ define a method
        // sim_main that can be called as the main function of
        // the program

        char *sopath = argv[1];
        void *handle = dlopen(sopath, RTLD_LAZY | RTLD_DEEPBIND);
        int (*sim_main)(int , char **) = dlsym(handle, "sim_main");

        int error = dlerror();
        if (error != NULL) {
                bsg_pr_err("Error when finding dynamically loaded symbol sim_main: %s\n", error);
                dlclose(handle);
                return error;
        }

        argv[1] = argv[0];
        int rc = (*sim_main)(argc-1, &argv[1]);

        dlclose(handle);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
