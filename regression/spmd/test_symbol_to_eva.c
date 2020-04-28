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

#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_loader.h>

#include <stdint.h>
#include <sys/stat.h>

#include "test_symbol_to_eva.h"

#define TEST_NAME_FMT "18s"

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

static int read_program_file(const char *file_name, unsigned char **file_data, size_t *file_size)
{
        struct stat st;
        FILE *f;
        int r;
        unsigned char *data;

        if ((r = stat(file_name, &st)) != 0) {
                bsg_pr_err("could not stat '%s': %m\n", file_name);
                return HB_MC_FAIL;
        }

        if (!(f = fopen(file_name, "rb"))) {
                bsg_pr_err("failed to open '%s': %m\n", file_name);
                return HB_MC_FAIL;
        }

        if (!(data = (unsigned char *) malloc(st.st_size))) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                return HB_MC_FAIL;
        }

        if ((r = fread(data, st.st_size, 1, f)) != 1) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                free(data);
                return HB_MC_FAIL;
        }

        fclose(f);
        *file_data = data;
        *file_size = st.st_size;
        return HB_MC_SUCCESS;
}

struct test_should_succeed {
        const char *test_name;
        const char *symbol;
        hb_mc_eva_t expected_eva;
};

/* add tests that should succeed here */
static struct test_should_succeed success_tests [] = {
        { "lookup-data", "_bsg_data_start_addr",    0x00001000 }, // start of DMEM I guess
        { "find-start",   "_start",                 0x00000000 }, // _start is now at 0
};

#define FTI_BAD_BUFFER   0x1
#define FTI_BAD_SIZE     0x2
#define FTI_BAD_EVA      0x4

struct test_should_fail {
        const char *test_name;
        uint16_t input;
        const char *symbol;
        int expected_result;
        size_t size;
};

/* add tests that should fail here */
static struct test_should_fail fail_tests [] = {
        { "bad-buffer",       FTI_BAD_BUFFER, "_bsg_data_start_addr",      HB_MC_INVALID  },
        { "symbol-not-found",              0, "@two-wild^and*crazy(guys?", HB_MC_NOTFOUND },
        { "bad-size",           FTI_BAD_SIZE, "_bsg_data_start_addr",      HB_MC_INVALID, 0  },
        { "badder-size",        FTI_BAD_SIZE, "_bsg_data_start_addr",      HB_MC_INVALID, 64 },
        { "bad-symbol",                    0,                   NULL,      HB_MC_INVALID },
        { "bad-eva",             FTI_BAD_EVA, "_bsg_data_start_addr",      HB_MC_INVALID },
};

int test_symbol_to_eva (int argc, char **argv) {
        unsigned char *program_data;
        size_t program_size;
        int err, r = HB_MC_SUCCESS;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        // read in the program data from the file system
        err = read_program_file(bin_path, &program_data, &program_size);
        if (err != HB_MC_SUCCESS)
                return err;

        /* now that the program has been loaded... */
        /* for each test that should succeed */
        for (int i = 0; i < array_size(success_tests); i++) {
                struct test_should_succeed *test = &success_tests[i];
                bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": ...\n", test_name, test->test_name);
                hb_mc_eva_t eva;
                err = hb_mc_loader_symbol_to_eva(program_data, program_size, test->symbol,
                                                 &eva);
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": symbol '%s': " BSG_RED("FAILED") ": %s\n",
                                         test_name,
                                         test->test_name,
                                         test->symbol,
                                         hb_mc_strerror(err));
                        r = HB_MC_FAIL;
                } else if (eva != test->expected_eva) {
                        bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": symbol '%s': " BSG_RED("FAILED") ": "
                                         "expected 0x%08" PRIx32 ", "
                                         "got 0x%08" PRIx32 "\n",
                                         test_name,
                                         test->test_name,
                                         test->symbol,
                                         test->expected_eva,
                                         eva);
                        r = HB_MC_FAIL;
                } else { // success!
                        bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": " BSG_GREEN("PASSED") "\n",
                                         test_name, test->test_name);
                }
        }

        /* for each test that should fail */
        for (int i = 0; i < array_size(fail_tests); i++) {
                struct test_should_fail *test = &fail_tests[i];
                hb_mc_eva_t eva, *eva_p;
                unsigned char *buffer;
                size_t sz;

                bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": ...\n", test_name, test->test_name);

                /* set inputs */
                if (test->input & FTI_BAD_BUFFER) {
                        buffer = NULL;
                } else {
                        buffer = program_data;
                }

                if (test->input & FTI_BAD_SIZE) {
                        sz = test->size;
                } else {
                        sz = program_size;
                }

                if (test->input & FTI_BAD_EVA) {
                        eva_p = NULL;
                } else {
                        eva_p = &eva;
                }

                err = hb_mc_loader_symbol_to_eva(buffer, sz, test->symbol,
                                                eva_p);
                if (err != test->expected_result) {
                        bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": " BSG_RED("FAILED") ": "
                                         "expected %s, "
                                         "got %s\n",
                                         test_name, test->test_name,
                                         hb_mc_strerror(test->expected_result),
                                         hb_mc_strerror(err));
                        r = HB_MC_FAIL;
                } else {
                        bsg_pr_test_info("%s: test %" TEST_NAME_FMT ": " BSG_GREEN("PASSED") "\n",
                                         test_name, test->test_name);
                }
        }

        return r;
}


#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info("test_symbol_to_eva Regression Test \n");
        int rc = test_symbol_to_eva(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


