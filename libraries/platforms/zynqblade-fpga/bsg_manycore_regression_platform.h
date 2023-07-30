#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Defined by the linker script for riscv64-unknown-elf-dramfs-gcc

// To make your program HammerBlade cross-platform compatible,
// define a function with the signature of "main", and then
// use this macro to mark it as the entry point of your program
//
// Example:
//
//    int MyMain(int argc, char *argv[]) {
//        /* your code here */
//    }
//    declare_program_main("The name of your test", MyMain)
//
#define declare_program_main(test_name, name)                  \
                                                               \
        int   _argc = __init_argc;                             \
        char _argv_flat[100]; /* Up to 100 characters */       \
        char* _argv[10];      /* Up to ten arguments  */       \
    int main() {                                               \
        int i = 0;                                             \
        int j = 0;                                             \
	_argv[j++] = &_argv_flat[i];                           \
        do {                                                   \
          if (__init_argv[i] == ' ') {                         \
            _argv_flat[i] = '\0';                              \
            _argv[j++] = &_argv_flat[i+1];                     \
	  } else {                                             \
            _argv_flat[i] = __init_argv[i];                    \
          }                                                    \
        } while (__init_argv[i++] != '\0');                    \
        bsg_pr_test_info("Regression Test: %s\n", test_name);  \
        int rc = name(_argc, _argv);                           \
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);            \
        return rc;                                             \
    }


#ifdef __cplusplus
}
#endif
