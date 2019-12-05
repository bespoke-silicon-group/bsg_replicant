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

#ifndef _CL_MANYCORE_REGRESSION_H
#define _CL_MANYCORE_REGRESSION_H

#ifdef __cplusplus

#include <cstdlib>
#include <cstdio>
#include <cstdbool>
#include <cfloat>

using std::isnormal;

#else

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <float.h>

#endif

#include <bsg_manycore_errno.h>
#include <unistd.h>
#include <argp.h>

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
#define bsg_pr_test_err(fmt, ...)                                       \
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


// Maximum tolerable FP error between X86 and RISCV 
#define MAX_FLOAT_ERROR_TOLERANCE 1e-6


// A data structure to generate random floating points 
// within the entire range
typedef union data_t {
        int32_t hb_mc_int;
        float hb_mc_float;
} hb_mc_data_t;

// Converts an int into a float and returns it
static inline float hb_mc_int_to_float(int i){ 
        hb_mc_data_t data;
        data.hb_mc_int = i;
        return data.hb_mc_float;
}

// Converts a float into an int and returns it
static inline int hb_mc_float_to_int (float f){
        hb_mc_data_t data;
        data.hb_mc_float = f;
        return data.hb_mc_int;
}
        

// Generates random floating point numbers
// within the permitted float32 range 
// Number has a 50% chance of being negative 
static inline float hb_mc_generate_float_rand(){
        hb_mc_data_t data;
        do
        {
                data.hb_mc_int = rand() * (((float)rand() / RAND_MAX) > 0.5 ? 1 : -1);
        } while(!isnormal(data.hb_mc_float));
        return data.hb_mc_float;
}

// Generates random floating point numbers
// within the permitted float32 range 
// All numbers of positive 
static inline float hb_mc_generate_float_rand_positive(){ 
        hb_mc_data_t data;
        do
        {
                data.hb_mc_int = rand();
        } while(!isnormal(data.hb_mc_float) || data.hb_mc_float < 0);
        return data.hb_mc_float;
}


// Compares two floating points and returns the relative error
// If the original number is zero, returns the differnce to avoid div by zero
// TODO: find a more appropriate solution for a = 0
static inline float hb_mc_calculate_float_error (float expected, float result) { 
        // if expected is close enough to zero that the division would return 1
        if (fabs(expected) < 1e-15)
                return (fabs(expected - result));
        return (fabs((float)(expected - result)/expected));
}

static inline bool hb_mc_floats_match(float a, float b) {
        float abs_a = fabs(a);
        float abs_b = fabs(b);
        float diff = fabs(a - b);
        hb_mc_data_t pun;
        pun.hb_mc_int = 0x00800000;
        float min_normal = pun.hb_mc_float;

        if(a == b || (isnan(abs_a) && isnan(abs_b)))
                return true;
        else if(a == 0 || b == 0 || (abs_a + abs_b < min_normal))
                return diff < (MAX_FLOAT_ERROR_TOLERANCE * min_normal);

        return (diff / fminf(abs_a + abs_b, FLT_MAX)) < MAX_FLOAT_ERROR_TOLERANCE;
}
#ifdef __cplusplus
extern "C" {
void cosim_main(uint32_t *exit_code, char * args);
}
#endif

#ifdef COSIM
// Given a string, determine the number of space-separated arguments
static
int get_argc(char * args){
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
#endif // COSIM
static char doc[] = "A regression test for BSG Manycore on F1";

struct arguments_none{
};

struct arguments_name{
        char *testname; // Name of RISC-V Manycore Binary to run
};

struct arguments_path{
        char *path; // Path to RISC-V Manycore Binary to run
        char *name; // Name of Test to Run
};

/*
  args_doc: A description of the non-option command-line arguments that we
  accept.
*/
static char desc_name[] = "<Test Name>";
static char desc_path[] = "<Path to Manycore Binary> <Name of Test>";
static char desc_path_py[] = "<Path to Python Tests Directory> <Name of Test>";
static char desc_none[] = "";
static struct argp_option opts_name[] = {
        {0, 'b', "TEST", 0, "Name of Manycore Test to Run"},
        {0}};
static struct argp_option opts_path[] = {
        {0, 'n', "NAME", 0, "Name of Manycore Test to Run"},
        {0, 'p', "PATH", 0, "Path to RISC-V Manycore Binary"},
        {0}};
static struct argp_option opts_path_py[] = {
        {0, 'n', "NAME", 0, "Name of Manycore Test to Run"},
        {0, 'p', "PATH", 0, "Path to Python Folder"},
        {0}};
static struct argp_option opts_none[] = {{0}};

static error_t parse_name (int key, char *arg, struct argp_state *state){
        struct arguments_name *args = (struct arguments_name *)state->input;
 
        switch (key) 
                {
                case 'b':
                        args->testname = arg;
                        break;
                case ARGP_KEY_ARG:
                        if (state->arg_num == 0){
                                args->testname = arg;
                        }
                        if (state->arg_num > 1){
                                bsg_pr_test_err("Too Many Arguments provided!\n");
                                argp_usage(state);
                        }
                        break;
                case ARGP_KEY_END:
                        if (!args->testname){
                                bsg_pr_test_err("Test Name not provided!\n");
                                argp_usage(state);
                        }
                        break;
                default:
                        return ARGP_ERR_UNKNOWN;
                }
        return 0;
}

static error_t parse_path (int key, char *arg, struct argp_state *state){
        struct arguments_path *args = (struct arguments_path *)state->input;
 
        switch (key) 
                {
                case 'p':
                        args->path = arg;
                        break;
                case 'n':
                        args->name = arg;
                        break;
                case ARGP_KEY_ARG:
                        if (state->arg_num == 0){
                                args->path = arg;
                        }
                        if (state->arg_num == 1){
                                args->name = arg;
                        }
                        if (state->arg_num > 2){
                                bsg_pr_test_err("Too Many Arguments provided!\n");
                                argp_usage(state);
                        }
                        break;
                case ARGP_KEY_END:
                        if (!args->path){
                                bsg_pr_test_err("Executable path not provided!\n");
                                argp_usage(state);
                        }
                        if (!args->name){
                                bsg_pr_test_err("Test Name not provided!\n");
                                argp_usage(state);
                        }
                        break;
                default:
                        return ARGP_ERR_UNKNOWN;
                }
        return 0;
}

static error_t parse_path_py (int key, char *arg, struct argp_state *state){
        return parse_path(key, arg, state);
}

static error_t parse_none (int key, char *arg, struct argp_state *state){
 
        switch (key) 
                {
                case ARGP_KEY_ARG:
                        bsg_pr_test_err("Too Many Arguments provided!\n");
                        argp_usage(state);
                        break;
                default:
                        return ARGP_ERR_UNKNOWN;
                }
        return 0;
}

static struct argp argp_name = {opts_name, parse_name, desc_name, doc};
static struct argp argp_path = {opts_path, parse_path, desc_path, doc};
static struct argp argp_path_py = {opts_path_py, parse_path_py, desc_path_py, doc};
static struct argp argp_none = {opts_none, parse_none, desc_none, doc};

#endif
