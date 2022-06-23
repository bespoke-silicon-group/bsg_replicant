#pragma once
#ifdef __cplusplus
extern "C" {
#endif

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
#define declare_program_main(test_name, name)           \
    int main(int argc, char *argv[]) {                  \
        bsg_pr_test_info("Regression Test: %s\n");      \
        int rc = name(argc, argv);                      \
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);     \
        return rc;                                      \
    }


#ifdef __cplusplus
}
#endif
