#pragma once
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <argp.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ENOUGH_WITH_THIS_COLOR_NONESENSE
#define BSG_RED(x) \
    "\033[31m" x "\033[0m"
#define BSG_GREEN(x) \
    "\033[32m" x "\033[0m"
#define BSG_YELLOW(x) \
    "\033[33m" x "\033[0m"
#else
#define BSG_RED(x) \
    x
#define BSG_GREEN(x) \
    x
#define BSG_YELLOW(x) \
    x
#endif

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


/****************************/
/* Array comparison helpers */
/****************************/
int hb_mc_compare_arrays(int num_fields, const uint32_t *expected, const uint32_t *actual);
int hb_mc_compare_results(int num_fields, const char *desc[], const uint32_t *expected, const uint32_t *actual);

/**************************/
/* Floating point helpers */
/**************************/
// Maximum tolerable FP error between X86 and RISCV
#define MAX_FLOAT_ERROR_TOLERANCE 1e-6

// Converts an int into a float and returns it
float hb_mc_int_to_float(int i);
// Converts a float into an int and returns it
int   hb_mc_float_to_int (float f);
// Generates random floating point numbers
// within the permitted float32 range
// Number has a 50% chance of being negative
float hb_mc_generate_float_rand();
// Generates random floating point numbers
// within the permitted float32 range
// All numbers of positive
float hb_mc_generate_float_rand_positive();

// Compares two floating points and returns the relative error
// If the original number is zero, returns the differnce to avoid div by zero
// TODO: find a more appropriate solution for a = 0
float hb_mc_calculate_float_error (float expected, float result);

// Returns true if the a and b are close enough in value
// to be equal
int   hb_mc_floats_match(float a, float b);

/************************/
/* Command line parsing */
/************************/

// Do not expect command line arguments
extern struct argp argp_none;
struct arguments_none{
};

// Expect the name of a riscv binary
extern struct argp argp_name;
struct arguments_name{
        char *testname; // Name of RISC-V Manycore Binary to run
};

// Expect the name of a riscv binary and a test name
extern struct argp argp_path_py;
extern struct argp argp_path;
struct arguments_path{
        char *path; // Path to RISC-V Manycore Binary to run
        char *name; // Name of Test to Run
};

#ifdef __cplusplus
}
#endif

#include <bsg_manycore_regression_platform.h>
