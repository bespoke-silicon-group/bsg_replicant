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
#include <bsg_manycore_eva.h>

#include <sys/stat.h>

#include "test_bsg_loader_suite.h"

#define __BSG_STRINGIFY(arg) #arg
#define BSG_STRINGIFY(arg) __BSG_STRINGIFY(arg)
#define SUITE_NAME "test_bsg_loader_suite"

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))
/*
   Reads program data from a file into a buffer that read_program_file() allocates.
   Returns the data and size.
*/
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

// /path/to/spmd/bsg_loader_suite
#define SUITE_DIR BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/bsg_loader_suite"

/* our test structures */
typedef struct test {
        const char *name;
#define TN_FMT "18s"
        const char *program_file_name;
        hb_mc_manycore_t *mc;
        // used in finish packet test
        uint32_t magic;
        // used in npa_to_eva
        hb_mc_npa_t npa;
        uint32_t data;
        int addr_is_dram;
        int addr_is_host;
} test_t;

#define pr_test_failed(test, reason, ...)                               \
        bsg_pr_test_info("%s: %" TN_FMT ": " BSG_RED("FAILED") ": " reason, \
                         SUITE_NAME,                                    \
                         test->name,                                    \
                         ##__VA_ARGS__                                  \
                )

#define pr_test_passed(test, reason, ...)                               \
        bsg_pr_test_info("%s: %" TN_FMT ": " BSG_GREEN("PASSED") ": " reason, \
                         SUITE_NAME,                                    \
                         test->name,                                    \
                         ##__VA_ARGS__                                  \
                )                                                       \


static hb_mc_manycore_t manycore = {0}; // the manycore to instantiate

/* freezes a tile and loads a binary, used as basis for all tests */
/* on success the pointer returned by r_pdata needs to be freed by caller */
static int run_freeze_and_load(test_t *test,
                               hb_mc_coordinate_t target,
                               unsigned char **r_pdata,
                               size_t *r_psize)
{
        hb_mc_manycore_t *mc = test->mc;
        const char *program_file_name = test->program_file_name;
        const char *test_name = test->name;
        unsigned char *program_data;
        size_t program_size;
        int rc = HB_MC_FAIL, err;

        rc = read_program_file(program_file_name, &program_data, &program_size);
        if (rc != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to read program data\n");
                return rc;
        }
        /* initialize the tile */
        hb_mc_coordinate_t origin = target;

        // freeze the tile
        err = hb_mc_tile_freeze(mc, &target);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to freeze tile (%" PRId32 ",%" PRId32"): %s\n",
                               hb_mc_coordinate_get_x(target),
                               hb_mc_coordinate_get_y(target),
                               hb_mc_strerror(err)
                        );
                goto cleanup;
        }

        // set its origin
        err = hb_mc_tile_set_origin(mc, &target, &origin);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to set origin of (%" PRId32 ", %" PRId32 ") "
                               "to (%" PRId32 ", %" PRId32 "): %s\n",
                               hb_mc_coordinate_get_x(target),
                               hb_mc_coordinate_get_y(target),
                               hb_mc_coordinate_get_x(origin),
                               hb_mc_coordinate_get_y(origin),
                               hb_mc_strerror(err));
                goto cleanup;
        }

        /* load the program */
        err = hb_mc_loader_load(program_data, program_size,
                                mc, &default_map,
                                &target, 1);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to load binary '%s': %s\n",
                               program_file_name,
                               hb_mc_strerror(err));
                return err;
        }
        // success
        rc = HB_MC_SUCCESS;
        *r_pdata = program_data;
        *r_psize = program_size;
        goto done;

cleanup:
        free(program_data);
done:
        return rc;
}

/*******************************************************************************/
/* Tests that should successfully load a file and receive a finish/fail packet */
/*******************************************************************************/
#define FINISH_PACKET_TEST(tname, tfile)                                  \
        { .name = tname, .program_file_name = tfile, .mc = &manycore, .magic = 0x3AB4 }

#define FAIL_PACKET_TEST(tname, tfile)          \
        { .name = tname, .program_file_name = tfile, .mc = &manycore, .magic = 0x3AB6 }

/**************************************/
/* ADD NEW PACKET LOOPBACK TESTS HERE */
/**************************************/
static test_t finish_packet_tests [] = {
        FINISH_PACKET_TEST("loopback",          SUITE_DIR "/loopback/main.riscv"),
        FINISH_PACKET_TEST("loopback cache",    SUITE_DIR "/loopback_cache/main.riscv"),
        FAIL_PACKET_TEST(  "loopback fail",     SUITE_DIR "/loopback_fail/main.riscv"),
        FINISH_PACKET_TEST("loopback big text", SUITE_DIR "/loopback_big_text/main.riscv"),
        FINISH_PACKET_TEST("npa_to_eva",        SUITE_DIR "/npa_to_eva/main.riscv"),
};

static int run_finish_packet_test(test_t *test)
{
        hb_mc_manycore_t *mc = test->mc;
        const char *program_file_name = test->program_file_name;
        const char *test_name = test->name;
        unsigned char *program_data;
        size_t program_size;
        int rc = HB_MC_FAIL, err;
        hb_mc_coordinate_t target = hb_mc_config_get_origin_vcore(hb_mc_manycore_get_config(mc));

        err = run_freeze_and_load(test, target, &program_data, &program_size);
        if (err != HB_MC_SUCCESS)
                return rc;

        err = hb_mc_tile_unfreeze(mc, &target);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to unfreeze tile (%" PRId32", %" PRId32 "): %s\n",
                               hb_mc_coordinate_get_x(target),
                               hb_mc_coordinate_get_y(target),
                               hb_mc_strerror(err));
                goto cleanup;
        }

        usleep(100);

        /* wait for the finish packet */
        hb_mc_packet_t finish;
        err = hb_mc_manycore_packet_rx(mc, &finish, HB_MC_FIFO_RX_REQ, -1);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to receive packet: %s\n",
                               hb_mc_strerror(err));
                goto cleanup;
        }


        /* print out the packet */
        char buf[256];
        bsg_pr_test_info("%s: %" TN_FMT ": received manycore finish packet %s\n",
                         SUITE_NAME,
                         test->name,
                         hb_mc_request_packet_to_string(&finish.request, buf ,sizeof(buf)));


        uint32_t magic = test->magic;
        if (hb_mc_request_packet_get_addr(&finish.request) != magic) {
                pr_test_failed(test, "packet does not match finish packet 0x%08" PRIx32 "\n",
                               magic);
                goto cleanup;
        } else {
                pr_test_passed(test, "received correct packet!\n");
        }

        rc = HB_MC_SUCCESS;
cleanup:
        free(program_data);
done:
        return rc;
}

/*
   Run all off the tests that should succesfully load a program and receive
   a finish packet from the hardware.
*/
static int run_finish_packet_tests(void)
{
        int err, rc = HB_MC_SUCCESS;

        bsg_pr_test_info("%s: " BSG_RED("RUNNING PACKET LOOPBACK TESTS") "\n", SUITE_NAME);
        for (int i = 0; i < array_size(finish_packet_tests); i++) {
                err = run_finish_packet_test(&finish_packet_tests[i]);
                if (err != HB_MC_SUCCESS)
                        rc = HB_MC_FAIL;
                // continue running all tests
        }
        return rc;
}


/******************************************************************/
/* Tests that should:                                             */
/* 1. successfully load the npa_to_eva program                    */
/* 2. write values to the programs 'data' and 'addr' variables    */
/* 3. unfreeze the tile                                           */
/* 3. receive the finish packet                                   */
/* 4. check the address written to 'addr' and compare with 'data' */
/*                                                                */
/* These tests are meant to exercise our NPA to EVA translation.  */
/******************************************************************/


#define NPA_TO_EVA_TEST(tname, taddr, tdata)                            \
        { .name = tname, .program_file_name = SUITE_DIR "/npa_to_eva/main.riscv", \
                        .magic = 0x3AB4, .npa = taddr, .data = tdata, .mc = &manycore }

// for DRAM, coordinate is rewritten at runtime according to configuration
#define NPA_TO_EVA_DRAM_TEST(tname, epa, tdata)                         \
        { .name = tname, .program_file_name = SUITE_DIR "/npa_to_eva/main.riscv", \
                        .magic = 0x3AB4, .npa = HB_MC_NPA(0,0,epa), .data = tdata, \
                        .mc = &manycore,                                \
                        .addr_is_dram = 1, .addr_is_host = 0 }

#define NPA_TO_EVA_HOST_TEST(tname, epa, tdata)                         \
        { .name = tname, .program_file_name = SUITE_DIR "/npa_to_eva/main.riscv", \
                        .magic = 0x3AB4, .npa = HB_MC_NPA(0,0,epa), .data = tdata, \
                        .mc = &manycore,                                \
                        .addr_is_dram = 0, .addr_is_host = 1 }

#define NPA_TO_EVA_LOCAL_TEST(tname, x, y, epa, tdata)  \
        { .name = tname, .program_file_name = SUITE_DIR "/npa_to_eva/main.riscv", \
                        .magic = 0x3AB4, .npa = HB_MC_NPA(x, y, 0x1000 | epa), .data = tdata, \
                        .mc = &manycore,                                \
                        .addr_is_dram = 0, .addr_is_host = 0 }

/*********************************/
/* ADD NEW NPA TO EVA TESTS HERE */
/*********************************/
static test_t npa_to_eva_tests [] = {
        /* Local memory tests */
        NPA_TO_EVA_LOCAL_TEST("local mem middle",    0, 1, 1<<9, 0xDEADBEEF),
        NPA_TO_EVA_LOCAL_TEST("local mem neighbor",  0, 2,    0, 0xDEADBEEF),
        /* DRAM NPA tests */
        NPA_TO_EVA_DRAM_TEST("dram",      0,         0xDEADBEEF),
        NPA_TO_EVA_DRAM_TEST("dram+4K",   4*(1<<10), 0xCAFEBABE),
        NPA_TO_EVA_DRAM_TEST("dram+1M",   1*(1<<20), 0xABABABAB),
        /* Host NPA tests */
        NPA_TO_EVA_HOST_TEST("host F000", 0xF000,    0xCAFEBABE),
        NPA_TO_EVA_HOST_TEST("host 000C", 0x000C,    0x0000C0DA),
};

static hb_mc_idx_t test_get_dram_y(test_t *test, hb_mc_manycore_t *mc)
{
        return hb_mc_config_get_dram_low_y(hb_mc_manycore_get_config(mc));
}

static int run_npa_to_eva_test(test_t *test)
{
        /**************************************************************************************/
        /* Each test does the following:                                                      */
        /* 1. Load the npa_to_eva riscv executable to one tile.                               */
        /* 2. Write to its npa_to_eva_(data/addr) variables.                                  */
        /* 3. Unfreeze the tile.                                                              */
        /* 4. If the target address is the host, read a request packet.                       */
        /* 5. Wait for a finish packet.                                                       */
        /* 6. Compare the data at the target NPA with data written to npa_to_eva_data.        */
        /**************************************************************************************/

        hb_mc_manycore_t *mc = test->mc;
        const char *test_name = test->name;
        unsigned char *program_data;
        size_t program_size;
        int rc = HB_MC_FAIL, err;
        hb_mc_coordinate_t target = hb_mc_config_get_origin_vcore(hb_mc_manycore_get_config(mc));
        hb_mc_npa_t npa = test->npa;
        uint32_t data   = test->data;
        char npa_str [64];

        /* rewrite Y-coordinate if this is DRAM or Host */
        if (test->addr_is_dram) {
                hb_mc_idx_t y = test_get_dram_y(test, mc);
                hb_mc_npa_set_y(&npa, y);
        } else if (test->addr_is_host) {
                const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
                hb_mc_coordinate_t hostif = hb_mc_config_get_host_interface(config);
                hb_mc_npa_set_x(&npa, hb_mc_coordinate_get_x(hostif));
                hb_mc_npa_set_y(&npa, hb_mc_coordinate_get_y(hostif));
        } else { // if this is a vcore address add a base x-y
                const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
                hb_mc_coordinate_t tile = hb_mc_config_get_origin_vcore(config);
                hb_mc_npa_set_x(&npa, hb_mc_npa_get_x(&npa) + hb_mc_coordinate_get_x(tile));
                hb_mc_npa_set_y(&npa, hb_mc_npa_get_y(&npa) + hb_mc_coordinate_get_y(tile));
        }

        // format npa as a string
        hb_mc_npa_to_string(&npa, npa_str, sizeof(npa_str));

        bsg_pr_dbg("%s: Freezing and Loading\n", test_name);

        err = run_freeze_and_load(test, target, &program_data, &program_size);
        if (err != HB_MC_SUCCESS)
                return rc;

        /*
          Get the address of npa_to_eva_addr.
         */
        const char *addr_name = "npa_to_eva_addr", *data_name = "npa_to_eva_data";
        hb_mc_eva_t addr_addr, data_addr;
        err = hb_mc_loader_symbol_to_eva(program_data, program_size,
                                         addr_name, &addr_addr);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to find '%s' in program: %s\n",
                               addr_name, hb_mc_strerror(err));
                goto cleanup;
        }

        /*
          Get the address of npa_to_eva_data.
         */
        err = hb_mc_loader_symbol_to_eva(program_data, program_size,
                                         data_name, &data_addr);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to find '%s' in program: %s\n",
                               data_name, hb_mc_strerror(err));
                goto cleanup;
        }

        bsg_pr_dbg("%s: %s = %08" PRIx32 ", %s = %08" PRIx32 "\n",
                   test_name, addr_name, addr_addr, data_name, data_addr);

        /*
          Translate our target NPA to and EVA.
         */
        hb_mc_eva_t addr;
        size_t addr_bytes;
        err = hb_mc_npa_to_eva(mc,
                               &default_map,
                               &target,
                               &npa,
                               &addr,
                               &addr_bytes);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to translate %s: %s\n",
                               npa_str, hb_mc_strerror(err));
                goto cleanup;
        }

        bsg_pr_dbg("%s: %s translates to EVA %08" PRIx32 "\n",
                   test_name, npa_str, addr);


        /*
          Now write the produced EVA to the V-Core as well as the data for it to write.
          The V-Core should write the data assigned to npa_to_eva_data to the address
          assigned to npa_to_eva_addr.
         */
        bsg_pr_dbg("%s: Writing %08" PRIx32 " to %08" PRIx32 " (%s)\n",
                   test_name, addr, addr_addr, addr_name);

        err = hb_mc_manycore_eva_write(mc, &default_map, &target,
                                       &addr_addr, &addr, sizeof(addr));
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to write to EVA @ 0x%08" PRIx32 " (%s): %s\n",
                               addr_addr, addr_name, hb_mc_strerror(err));
                goto cleanup;
        }

        err = hb_mc_manycore_eva_write(mc, &default_map, &target,
                                       &data_addr, &data, sizeof(data));
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to write to EVA @0x%08" PRIx32 " (%s): %s\n",
                               data_addr, data_name, hb_mc_strerror(err));
                goto cleanup;
        }

        /*
          Release the tile so it can do the write.
         */
        bsg_pr_dbg("%s: Unfreezing (%" PRId8 ",%" PRId8 ")\n",
                   test_name,
                   hb_mc_coordinate_get_x(target),
                   hb_mc_coordinate_get_y(target));

        err = hb_mc_tile_unfreeze(mc, &target);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to freeze tile (%" PRId32 ",%" PRId32"): %s\n",
                               hb_mc_coordinate_get_x(target),
                               hb_mc_coordinate_get_y(target),
                               hb_mc_strerror(err)
                        );
                goto cleanup;
        }

        /*
          Give the V-Core time to run (100us is plenty of time).
         */
        usleep(100);

        uint32_t data_read;

        /*
          If the target NPA was a host address we should expect a remote store
          packet from the network.
         */
        if (test->addr_is_host) {
                // we should expect a write packet since the EVA we gave to the vcore
                // maps to the host
                bsg_pr_dbg("%s: Waiting for write packet from V-Core\n", test_name);

                hb_mc_packet_t rqst;
                err = hb_mc_manycore_packet_rx(mc, &rqst, HB_MC_FIFO_RX_REQ, -1);
                if (err != HB_MC_SUCCESS) {
                        pr_test_failed(test, "failed to receive write packet: %s\n",
                                       hb_mc_strerror(err));
                        goto cleanup;
                }
                        /* print out the packet */
                char buf[256];
                bsg_pr_test_info("%s: %" TN_FMT ": received manycore request packet %s\n",
                                 SUITE_NAME,
                                 test->name,
                                 hb_mc_request_packet_to_string(&rqst.request,
                                                                buf,
                                                                sizeof(buf))
                        );
                data_read = hb_mc_request_packet_get_data(&rqst.request);
        }

        /*
          We should now expect a finish packet.y
         */
        bsg_pr_dbg("%s: Waiting for finish packet\n", test_name);

        hb_mc_packet_t finish;
        err = hb_mc_manycore_packet_rx(mc, &finish, HB_MC_FIFO_RX_REQ, -1);
        if (err != HB_MC_SUCCESS) {
                pr_test_failed(test, "failed to receive packet: %s\n",
                               hb_mc_strerror(err));
                goto cleanup;
        }


        /* print out the packet */
        char buf[256];
        bsg_pr_test_info("%s: %" TN_FMT ": received manycore finish packet  %s\n",
                         SUITE_NAME,
                         test->name,
                         hb_mc_request_packet_to_string(&finish.request, buf ,sizeof(buf)));


        /*
          Make sure that the finish packet has the magic number we expect.
         */
        uint32_t magic = test->magic;
        if (hb_mc_request_packet_get_addr(&finish.request) != magic) {
                pr_test_failed(test, "packet does not match finish packet 0x%08" PRIx32 "\n",
                               magic);
                goto cleanup;
        }

        /*
          If the target NPA was somewhere other than the host, we should read from
          our target NPA and check that it has the expected value.
         */
        if (!test->addr_is_host) {
                // we should read from the NPA we gave the vcore
                err = hb_mc_manycore_read_mem(mc, &npa, &data_read, sizeof(data_read));
                if (err != HB_MC_SUCCESS) {
                        pr_test_failed(test, "failed to read data from %s: %s\n",
                                       npa_str, hb_mc_strerror(err));
                        goto cleanup;
                }
        }

        /*
           Finally compare read data with what we asked the vcore to write
        */
        if (data_read == data) {
                pr_test_passed(test, "Data read from %s matches 0x%08" PRIx32 "\n",
                               npa_str, data);
                rc = HB_MC_SUCCESS;
        } else {
                pr_test_failed(test, "Data read from %s (=0x%08" PRIx32 ") does not"
                               " match 0x%08" PRIx32 "\n",
                               npa_str, data_read, data);
                rc = HB_MC_FAIL;
        }

cleanup:
        free(program_data);

done:
        return rc;
}

static int run_npa_to_eva_tests()
{
        int rc = HB_MC_SUCCESS, err;

        bsg_pr_test_info("%s: " BSG_RED("RUNNING NPA TO EVA TESTS") "\n", SUITE_NAME);
        for (int i = 0; i < array_size(npa_to_eva_tests); i++) {
                err = run_npa_to_eva_test(&npa_to_eva_tests[i]);
                if (err != HB_MC_SUCCESS)
                        rc = HB_MC_FAIL;
                // continue running all tests
        }
        return rc;
}
/******************/
/* Main test loop */
/******************/
int test_loader_suite (int argc, char **argv) {
        int err, rc = HB_MC_SUCCESS;

        err = hb_mc_manycore_init(&manycore, SUITE_NAME, 0);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to initialize manycore: %s\n",
                             SUITE_NAME, hb_mc_strerror(err));
                return HB_MC_FAIL;
        }

        /***************************/
        /* Run finish packet tests */
        /***************************/
        err = run_finish_packet_tests();
        if (err != HB_MC_SUCCESS)
                rc = HB_MC_FAIL;

        /************************/
        /* Run NPA to EVA tests */
        /************************/
        err = run_npa_to_eva_tests();
        if (err != HB_MC_SUCCESS)
                rc = HB_MC_FAIL;

        hb_mc_manycore_exit(&manycore);

        return err;
}


#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
        // We aren't passed command line arguments directly so we parse them
        // from *args. args is a string from VCS - to pass a string of arguments
        // to args, pass c_args to VCS as follows: +c_args="<space separated
        // list of args>"
        int argc = get_argc(args);
        char *argv[argc];
        get_argv(args, argc, argv);

#ifdef VCS
        svScope scope;
        scope = svGetScopeFromName("tb");
        svSetScope(scope);
#endif
        bsg_pr_test_info(SUITE_NAME " Regression Test (COSIMULATION)\n");
        int rc = test_loader_suite(argc, argv);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char ** argv) {
        bsg_pr_test_info(SUITE_NAME " Regression Test (F1)\n");
        int rc = test_loader_suite(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif

