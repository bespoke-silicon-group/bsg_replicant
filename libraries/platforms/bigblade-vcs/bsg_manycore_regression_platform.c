#include <stdint.h>
#include <unistd.h>
#include <bsg_manycore_regression.h>
#include <dlfcn.h>
// Given a string, determine the number of space-separated arguments
static int get_argc(char * args){
        char *cur = args, prev=' ';
        int count = 1;
        while(*cur != '\0'){
                if((prev == ' ') && (prev != *cur)){
                        count ++;
                }
                prev = *cur;
                ++cur;
        }
        return count;
}

static
void get_argv(char * args, int argc, char **argv){
        int count = 0;
        char *cur = args, prev=' ';

        // First parse the path name. This is not in the argument string because
        // VCS doesn't provide it to us. Instead, we "hack" around it by reading
        // the path from 'proc/self/exe'. The maximum path-name length is 1024,
        // with an extra null character for safety
        static char path[1025] = {'\0'};

        readlink("/proc/self/exe", path, sizeof(path) - 1);
        argv[0] = path;
        count ++;

        // Then we parse the remaining arguments. Arguments are separated by N
        // >= 1 spaces. We only register an argument when the previous character
        // was a space, and the current character is not (so that multiple
        // spaces don't count as multiple arguments). We replace spaces with
        // null characters (\0) so that each argument appears to be an
        // individual string and can be used later, by argparse (and other
        // libraries)
        while(*cur != '\0'){
                if((prev == ' ') && (prev != *cur)){
                        argv[count] = cur;
                        count++;
                }
                prev = *cur;
                if(*cur == ' ')
                        *cur = '\0';
                cur++;
        }
}

// This function is the VCS hook for cosimulation
int covcs_main(uint32_t *exit_code, char *args, char *sopath) {
        // We aren't passed command line arguments directly so we parse them
        // from *args. args is a string from VCS - to pass a string of arguments
        // to args, pass c_args to VCS as follows: +c_args="<@separated
        // list of args>"
        // NOTE: For xcelium, we pass @ separated strings
        static char args_subst[1025];
        for (int i = 0; args[i] != '\0'; i++) {
                if((args[i] == '@')) {
                        args_subst[i] = ' ';
                } else {
                        args_subst[i] = args[i];
                }
        }

        int argc = get_argc(args_subst);
        char *argv[argc];
        char *error;
        get_argv(args_subst, argc, argv);

        // To reduce the number of VCS compilations we do, this
        // platform compiles the executable machine and then loads the
        // program as a shared object file. This shared object is
        // passed as the string sopath and _must_ define a method
        // vcs_main that can be called as the main function of the
        // program
        void *handle = dlopen(sopath, RTLD_LAZY | RTLD_DEEPBIND);
        int (*vcs_main)(int , char **) = dlsym(handle, "vcs_main");

        error = dlerror();
        if (error != NULL) {
                bsg_pr_err("Error when finding dynamically loaded symbol vcs_main: %s\n", error);
                *exit_code = -1;
                dlclose(handle);
                return 1;
        }

        int rc = (*vcs_main)(argc, argv);

        dlclose(handle);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return 0;
}
