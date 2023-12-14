#include <bsg_manycore_regression.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <float.h>
#include <argp.h>
#include <string.h>

/****************************/
/* Array comparison helpers */
/****************************/

// Compares two arrays and prints the indices of mismatching elements.
// @param num_fields The number of elements to compare
// @param expected The expected values to compare against
// @param actual The actual values returned by the test
int hb_mc_compare_arrays(int num_fields, const uint32_t *expected, const uint32_t *actual) {
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
int hb_mc_compare_results(int num_fields, const char *desc[], const uint32_t *expected, const uint32_t *actual) {
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

/**************************/
/* Floating point helpers */
/**************************/

// A data structure to generate random floating points
// within the entire range
typedef union data_t {
        int32_t hb_mc_int;
        float hb_mc_float;
} hb_mc_data_t;

// Converts an int into a float and returns it
float hb_mc_int_to_float(int i){
        hb_mc_data_t data;
        data.hb_mc_int = i;
        return data.hb_mc_float;
}

// Converts a float into an int and returns it
int hb_mc_float_to_int (float f){
        hb_mc_data_t data;
        data.hb_mc_float = f;
        return data.hb_mc_int;
}


// Generates random floating point numbers
// within the permitted float32 range
// Number has a 50% chance of being negative
float hb_mc_generate_float_rand(){
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
float hb_mc_generate_float_rand_positive(){
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
float hb_mc_calculate_float_error (float expected, float result) {
        // if expected is close enough to zero that the division would return 1
        if (fabs(expected) < 1e-15)
                return (fabs(expected - result));
        return (fabs((float)(expected - result)/expected));
}

// Returns true if the a and b are close enough in value
// to be equal
int hb_mc_floats_match(float a, float b) {
        float abs_a = fabs(a);
        float abs_b = fabs(b);
        float diff = fabs(a - b);
        hb_mc_data_t pun;
        pun.hb_mc_int = 0x00800000;
        float min_normal = pun.hb_mc_float;

        if(a == b || (isnan(abs_a) && isnan(abs_b)))
                return 1;

        else if(a == 0 || b == 0 || (abs_a + abs_b < min_normal))
                return diff < (MAX_FLOAT_ERROR_TOLERANCE * min_normal);

        return (diff / fminf(abs_a + abs_b, FLT_MAX)) < MAX_FLOAT_ERROR_TOLERANCE;
}

/************************/
/* Command line parsing */
/************************/
static char doc[] = "A regression test for BSG Manycore on F1";

/*
  args_doc: A description of the non-option command-line arguments that we
  accept.
*/
static char desc_name[] = "<Test Name> <Hardware device ID>";
static char desc_path[] = "<Path to Manycore Binary> <Name of Test> <Hardware device ID>";
static char desc_spmd[] = "<Path to Manycore Binary> <Name of Test> <Tile Group Dimension X> <Tile Group Dimension Y> <Hardware device ID>";
static char desc_path_py[] = "<Path to Python Tests Directory> <Name of Test> <Hardware device ID>";
static char desc_none[] = "<Hardware device ID>";

static struct argp_option opts_name[] = {
        {0, 'b', "TEST", 0, "Name of Manycore Test to Run"},
        {0, 'd',   "ID", 0, "Hardware device ID"},
        {0}};

static struct argp_option opts_path[] = {
        {0, 'n', "NAME", 0, "Name of Manycore Test to Run"},
        {0, 'p', "PATH", 0, "Path to RISC-V Manycore Binary"},
        {0, 'd',   "ID", 0, "Hardware device ID"},
        {0}};

static struct argp_option opts_spmd[] = {
        {0, 'n', "NAME", 0, "Name of Manycore Test to Run"},
        {0, 'p', "PATH", 0, "Path to RISC-V Manycore Binary"},
        {0, 'x', "TG_X", 0, "Tile Group X Dimension"},
        {0, 'y', "TG_Y", 0, "Tile Group Y Dimension"},
        {0, 'd',   "ID", 0, "Hardware device ID"},
        {0}};

static struct argp_option opts_path_py[] = {
        {0, 'n', "NAME", 0, "Name of Manycore Test to Run"},
        {0, 'p', "PATH", 0, "Path to Python Folder"},
        {0, 'd',   "ID", 0, "Hardware device ID"},
        {0}};

static struct argp_option opts_none[] = {
        {0, 'd',   "ID", 0, "Hardware device ID"},
        {0}};

static error_t parse_name (int key, char *arg, struct argp_state *state){
        struct arguments_name *args = (struct arguments_name *)state->input;

        switch (key)
                {
                case 'b':
                        args->testname = arg;
                        break;
                case 'd':
                        args->device_id = atoi(arg);
                        break;
                case ARGP_KEY_ARG:
                        if (state->arg_num == 0){
                                args->testname = arg;
                        }
                        if (state->arg_num == 1){
                                args->device_id = atoi(arg);
                        }
                        if (state->arg_num > 2){
                                bsg_pr_test_err("Too Many Arguments provided!\n");
                                argp_usage(state);
                        }
                        break;
                case ARGP_KEY_END:
                        if (!args->testname){
                                bsg_pr_test_err("Test Name not provided!\n");
                                argp_usage(state);
                        }
                        if (args->device_id < -1){
                                bsg_pr_test_err("Hardware device ID not valid!\n");
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
                case 'd':
                        args->device_id = atoi(arg);
                        break;
                case ARGP_KEY_ARG:
                        if (state->arg_num == 0){
                                args->path = arg;
                        }
                        if (state->arg_num == 1){
                                args->name = arg;
                        }
                        if (state->arg_num == 2){
                                args->device_id = atoi(arg);
                        }
                        if (state->arg_num > 3){
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
                        if (args->device_id < -1){
                                bsg_pr_test_err("Hardware device ID not valid!\n");
                                argp_usage(state);
                        }
                        break;
                default:
                        return ARGP_ERR_UNKNOWN;
                }
        return 0;
}

static error_t parse_spmd (int key, char *arg, struct argp_state *state){
        struct arguments_spmd *args = (struct arguments_spmd *)state->input;

        switch (key)
                {
                case 'p':
                        args->path = arg;
                        break;
                case 'n':
                        args->name = arg;
                        break;
                case 'x':
                        args->tg_x = arg;
                        break;
                case 'y':
                        args->tg_y = arg;
                        break;
                case 'd':
                        args->device_id = atoi(arg);
                        break;
                case ARGP_KEY_ARG:
                        if (state->arg_num == 0){
                                args->path = arg;
                        }
                        if (state->arg_num == 1){
                                args->name = arg;
                        }
                        if (state->arg_num == 2){
                                args->tg_x = atoi(arg);
                        }
                        if (state->arg_num == 3){
                                args->tg_y = atoi(arg);
                        }
                        if (state->arg_num == 4){
                                args->device_id = atoi(arg);
                        }
                        if (state->arg_num > 5){
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
                        if (args->tg_x > 32){
                                bsg_pr_test_err("Tile Group X dimension not valid!\n");
                                argp_usage(state);
                        }
                        if (args->tg_y > 32){
                                bsg_pr_test_err("Tile Group X dimension not valid!\n");
                                argp_usage(state);
                        }
                        if (args->device_id < -1){
                                bsg_pr_test_err("Hardware device ID not valid!\n");
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
        struct arguments_none *args = (struct arguments_none *)state->input;

        switch (key)
                {
                case 'd':
                        args->device_id = atoi(arg);
                        break;
                case ARGP_KEY_ARG:
                        if (state->arg_num == 0){
                                args->device_id = atoi(arg);
                        }
                        if (state->arg_num > 1){
                                bsg_pr_test_err("Too Many Arguments provided!\n");
                                argp_usage(state);
                        }
                        break;
                case ARGP_KEY_END:
                        if (args->device_id < -1){
                                bsg_pr_test_err("Hardware device ID not valid!\n");
                                argp_usage(state);
                        }
                        break;
                default:
                        return ARGP_ERR_UNKNOWN;
                }
        return 0;
}

struct argp argp_name = {opts_name, parse_name, desc_name, doc};
struct argp argp_path = {opts_path, parse_path, desc_path, doc};
struct argp argp_spmd = {opts_spmd, parse_spmd, desc_spmd, doc};
struct argp argp_path_py = {opts_path_py, parse_path_py, desc_path_py, doc};
struct argp argp_none = {opts_none, parse_none, desc_none, doc};
