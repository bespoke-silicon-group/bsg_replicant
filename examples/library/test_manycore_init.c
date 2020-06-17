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

#include <bsg_manycore_printing.h>
#include <bsg_manycore.h>
#include "test_manycore_init.h"

#define TEST_NAME "test_manycore_init"

#define test_pr_err(fmt, ...)                           \
    bsg_pr_err(TEST_NAME ": " fmt, ##__VA_ARGS__)

#define test_pr_info(fmt, ...)                          \
    bsg_pr_test_info(TEST_NAME ": " fmt, ##__VA_ARGS__)

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

typedef enum mcptrval {
    MANYCORE_PTR_DEFAULT = 0,
    MANYCORE_PTR_NULL,
    MANYCORE_PTR_INITIALIZED,
} manycore_ptr_value;

static const char * manycore_ptr_value_to_string(manycore_ptr_value ptrval)
{
    static const char *strtab [] = {
        [MANYCORE_PTR_DEFAULT]     = "uninitialized",
        [MANYCORE_PTR_NULL]        = "null",
        [MANYCORE_PTR_INITIALIZED] = "initialized",
    };

    return strtab[ptrval];
}

struct test {
    const char *name;
    struct {
        manycore_ptr_value mc_ptr;
        const char *mc_name;
        int mc_id;
    } manycore_init_input;
    struct {
        int mc_err;
    } manycore_init_output;
};

/*******************************/
/* Add new tests to this array */
/*******************************/
static struct test tests [] = {
    {
        .name = "nullptr-input",
        .manycore_init_input  = { MANYCORE_PTR_NULL, "null", 0},
        .manycore_init_output = { HB_MC_INVALID },
    },
    {
        .name = "non-zero-id",
        .manycore_init_input  = { MANYCORE_PTR_DEFAULT, "nzero", 1},
        .manycore_init_output = { HB_MC_INVALID },
    },
    {
        .name = "no-name",
        .manycore_init_input = { MANYCORE_PTR_DEFAULT, 0, 0},
        .manycore_init_output = { HB_MC_INVALID },
    },
    {
        .name = "init-twice",
        .manycore_init_input = { MANYCORE_PTR_INITIALIZED, "twice", 0},
        .manycore_init_output = { HB_MC_INITIALIZED_TWICE },
    },
    {
        .name = "good-input",
        .manycore_init_input = { MANYCORE_PTR_DEFAULT, "good", 0},
        .manycore_init_output = { HB_MC_SUCCESS },
    },
};

static
int test_manycore_init(void)
{
    int testno, fail = 0, err;

    hb_mc_manycore_t initialized = {0};
    err = hb_mc_manycore_init(&initialized, "initialized", 0);
    if (err != HB_MC_SUCCESS) {
        test_pr_err(BSG_RED("ERROR") " while initializing test suite: %s\n",
                    hb_mc_strerror(err));
        return err;
    }
    
    // for each test
    for (testno = 0; testno < array_size(tests); testno++) {    
        struct test *test = &tests[testno];
        hb_mc_manycore_t manycore = {0};

        switch (test->manycore_init_input.mc_ptr) {
        case MANYCORE_PTR_NULL:
            err = hb_mc_manycore_init(0,
                                      test->manycore_init_input.mc_name,
                                      test->manycore_init_input.mc_id);
            break;
        case MANYCORE_PTR_INITIALIZED:
            err = hb_mc_manycore_init(&initialized,
                                      test->manycore_init_input.mc_name,
                                      test->manycore_init_input.mc_id);
            break;          
        case MANYCORE_PTR_DEFAULT:
        default:
            err = hb_mc_manycore_init(&manycore,
                                      test->manycore_init_input.mc_name,
                                      test->manycore_init_input.mc_id);
            break;
        }
        
        const char *status = (err == test->manycore_init_output.mc_err
                              ? BSG_GREEN("PASSED")
                              : BSG_RED("FAILED"));

        fail = fail || (err != test->manycore_init_output.mc_err);
        
        // determine if the test passed or failed
        test_pr_info("Test %15s %s: "
                     "Called hb_mc_manycore_init(%s,%s,%d), "
                     "Expected %s, "
                     "Returned %s\n",
                     test->name,
                     status,
                     manycore_ptr_value_to_string(test->manycore_init_input.mc_ptr),
                     test->manycore_init_input.mc_name,
                     test->manycore_init_input.mc_id,
                     hb_mc_strerror(test->manycore_init_output.mc_err),
                     hb_mc_strerror(err));
        
        // cleanup if init succeeded
        if (err == HB_MC_SUCCESS &&
            (test->manycore_init_input.mc_ptr != MANYCORE_PTR_INITIALIZED))         
            hb_mc_manycore_exit(&manycore);
    }
    
    return fail ? HB_MC_FAIL : HB_MC_SUCCESS;
}
#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info(TEST_NAME " Regression Test \n");
        int rc = test_manycore_init();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

