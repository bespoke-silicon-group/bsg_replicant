#ifndef _CL_MANYCORE_REGRESSION_H
#define _CL_MANYCORE_REGRESSION_H
#include <stdio.h>
#include <stdint.h>
#include <bsg_manycore_errno.h>
#include <unistd.h>
#ifdef VCS
#include "svdpi.h"
#endif
#define BSG_RED(x) "\033[31m" x "\033[0m"
#define BSG_GREEN(x) "\033[32m" x "\033[0m"
#define BSG_YELLOW(x) "\033[33m" x "\033[0m"

/**
 * bsg_pr_test_info() a version of printf. All regression tests should prefer this function over direct stdio calls.
 */
#define bsg_pr_test_info(fmt, ...)                                      \
    do { printf("BSG INFO: " fmt, ##__VA_ARGS__); fflush(NULL); } while (0)

/**
 * bsg_pr_test_info() a version of printf(stderr,...). All regression tests should prefer this function over direct stdio calls.
 */
#define bsg_pr_test_err(fmt, ...)                                      \
    fprintf(stderr, BSG_RED("BSG ERR: " fmt), ##__VA_ARGS__)

/**
 * bsg_pr_test_pass_fail() prints a success/fail message depending on a test condition
 * @param[in] success_condition a condition which, if true, indicates that the test has passed
 */
#define bsg_pr_test_pass_fail(success_condition)                        \
        printf("BSG REGRESSION TEST %s\n", ((success_condition) ? BSG_GREEN("PASSED") : BSG_RED("FAILED")))

// Compares two arrays and prints the indices of mismatching elements. 
// @param num_fields The number of elements to compare
// @param expected The expected values to compare against
// @param actual The actual values returned by the test
static
int compare_arrays(int num_fields, const uint32_t *expected, const uint32_t *actual) {
        int num_bad_indices = 0;
        int bad_indices[num_fields];
        for(int i = 0; i < num_fields; i++) {
                bsg_pr_test_info("%d: Expected = %u, Actual = %u", i, expected[i], actual[i]);
                if(expected[i] != actual[i]) {
                        bad_indices[num_bad_indices++] = i;
                        printf(BSG_RED(" Failed\n"));
                }
                else
                        printf(BSG_GREEN(" Success\n"));
        }
        if(num_bad_indices == 0) {
                bsg_pr_test_info("Comparison successful");
                return HB_MC_SUCCESS;
        }
        bsg_pr_test_info("Comparison failed at indices:\n");
        for(int i = 0; i < num_bad_indices; i++)
                printf("%d ", bad_indices[i]);
        printf("\n");
        return HB_MC_FAIL;
}

// Compares two arrays with their descriptions. Prints the expected and actual, and whether the comparison 
// succeeded for each line.
// @param num_fields The number of elements to compare
// @param desc The description strings of each field
// @param expected The expected values to compare against
// @param actual The actual values returned by the test
// @returns HB_MC_SUCCESS if all fields at corresponding indices in expected and actual match, HB_MC_FAIL otherwise
static
int compare_results(int num_fields, const char *desc[], const uint32_t *expected, const uint32_t *actual) {
        int success = 1;
        for(int i = 0; i < num_fields; i++) {
                bsg_pr_test_info("%s: Expected = %u, Actual = %u", desc[i], expected[i], actual[i]);
                if(expected[i] != actual[i]) {
                        printf(BSG_RED(" Failed\n"));
                        success = 0;
                } 
                else
                        printf(BSG_GREEN(" Success\n"));
        }
        return success ? HB_MC_SUCCESS : HB_MC_FAIL;
}

void cosim_main(uint32_t *exit_code, char * args);
// Given a string, determine the number of space-separated arguments
static
int get_argc(char * args){
        char *cur = args, last=' ';
        int count = 1;
        while(*cur != '\0'){
                if((last == ' ') && (last != *cur)){
                        count ++;
                        printf("%s\n", cur);
                }
                last = *cur;
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

        readlink("/proc/self/exe", path, sizeof(path));
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
#endif
