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
#include <bsg_manycore_npa.h>
#include <bsg_manycore_printing.h>
#include <cinttypes>
#include <type_traits>
#include "test_manycore_dmem_read_write.hpp"

#define TEST_NAME "test_manycore_dmem_read_write"

int test_mem_functions(hb_mc_manycore_t *mc) {
        int err;
        /**************************/
        /* Writing to Data Memory */
        /**************************/
        uint32_t write_data = rand();
        hb_mc_npa_t npa = {
		.x = hb_mc_config_get_vcore_base_x(hb_mc_manycore_get_config(mc)),
		.y = hb_mc_config_get_vcore_base_y(hb_mc_manycore_get_config(mc)),
		.epa = DMEM_BASE
	};

        bsg_pr_test_info("Writing to DMEM\n");
        err = hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data));
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to write to manycore DMEM: %s\n",
                           __func__,
                           hb_mc_strerror(err));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info("Write successful\n");

        /******************************/
        /* Read back Data from Memory */
        /******************************/
        uint32_t read_data;
        err = hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data));
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to read from manycore DMEM: %s\n",
                           __func__,
                           hb_mc_strerror(err));
                return HB_MC_FAIL;
        }

        bsg_pr_test_info("Completed read\n");
        if (read_data == write_data) {
                bsg_pr_test_info("Read back data written: 0x%08" PRIx32 "\n",
                                 read_data);
        } else {
                bsg_pr_test_info("Data mismatch: read 0x%08" PRIx32 ", wrote 0x%08" PRIx32 "\n",
                                 read_data, write_data);
        }
        return read_data == write_data ? HB_MC_SUCCESS : HB_MC_FAIL;
}

template <typename T>
int test_read_write(hb_mc_manycore_t *mc) {
        constexpr bool is32 = std::is_same<T, uint32_t>::value;
        constexpr bool is16 = std::is_same<T, uint16_t>::value;
        constexpr bool is8  = std::is_same<T,  uint8_t>::value;
        static_assert(is32 || is16 || is8,                      
                      "Only uint32_t, uint16_t, or uint8_t are supported for testing!");
        using read_fn_t  = typename std::add_pointer<int(hb_mc_manycore_t *, const hb_mc_npa_t *, T *)>::type;
        using write_fn_t = typename std::add_pointer<int(hb_mc_manycore_t *, const hb_mc_npa_t *, T  )>::type;

        // NOTE: We can get rid of these reinterpret_casts if we use if constexpr in C++17
        constexpr read_fn_t read_fn =
                is32 ? reinterpret_cast<read_fn_t>(hb_mc_manycore_read32) :
                is16 ? reinterpret_cast<read_fn_t>(hb_mc_manycore_read16) :
                reinterpret_cast<read_fn_t>(hb_mc_manycore_read8);
        constexpr write_fn_t write_fn = 
                is32 ? reinterpret_cast<write_fn_t>(hb_mc_manycore_write32) :
                is16 ? reinterpret_cast<write_fn_t>(hb_mc_manycore_write16) :
                reinterpret_cast<write_fn_t>(hb_mc_manycore_write8);

        constexpr int num_iter = is32 ? 1 : is16 ? 2 : 4;
        int err;
        /**************************/
        /* Writing to Data Memory */
        /**************************/

        for(int i = 0; i < num_iter; i++)
        {
                T write_data = static_cast<T>(rand());
                hb_mc_npa_t npa = {
			.x = hb_mc_config_get_vcore_base_x(hb_mc_manycore_get_config(mc)),
			.y = hb_mc_config_get_vcore_base_y(hb_mc_manycore_get_config(mc)),
			.epa = DMEM_BASE + sizeof(T) * i
		};
                
                bsg_pr_test_info("Writing %u bytes to DMEM address 0x%08" PRIx32 "\n", sizeof(T), npa.epa);
                err = write_fn(mc, &npa, write_data);
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to write to manycore DMEM: %s\n",
                                   __func__,
                                   hb_mc_strerror(err));
                        return HB_MC_FAIL;
                }
                bsg_pr_test_info("Write successful\n");
                
                /******************************/
                /* Read back Data from Memory */
                /******************************/
                T read_data;
                err = read_fn(mc, &npa, &read_data);
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to read from manycore DMEM: %s\n",
                                   __func__,
                                   hb_mc_strerror(err));
                        return HB_MC_FAIL;
                }
                
                bsg_pr_test_info("Completed read\n");
                if (read_data == write_data) {
                        bsg_pr_test_info("Read back data written: 0x%08" PRIx32 "\n",
                                         read_data);
                } else {
                        bsg_pr_test_info("Data mismatch: read 0x%08" PRIx32 ", wrote 0x%08" PRIx32 "\n",
                                         read_data, write_data);
                        return HB_MC_FAIL;
                }
        }
        return HB_MC_SUCCESS;
}

int test_manycore_dmem_read_write () {
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;

        srand(0xBEEF);

        /********/
        /* INIT */
        /********/
        int err = hb_mc_manycore_init(mc, TEST_NAME, 0);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to initialize manycore: %s\n",
                           __func__, hb_mc_strerror(err));
                return HB_MC_FAIL;
        }

        int r = HB_MC_FAIL;
        if(test_mem_functions(mc) != HB_MC_SUCCESS)
                goto cleanup;
        if(test_read_write<uint8_t>(mc) != HB_MC_SUCCESS)
                goto cleanup;
        if(test_read_write<uint16_t>(mc) != HB_MC_SUCCESS)
                goto cleanup;
        if(test_read_write<uint32_t>(mc) != HB_MC_SUCCESS)
                goto cleanup;
        r = HB_MC_SUCCESS;
        /*******/
        /* END */
        /*******/
cleanup:
        hb_mc_manycore_exit(mc);
        return r;
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
        bsg_pr_test_info(TEST_NAME " Regression Test (COSIMULATION)\n");
        int rc = test_manycore_dmem_read_write();
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char ** argv) {
        bsg_pr_test_info(TEST_NAME " Regression Test (F1)\n");
        int rc = test_manycore_dmem_read_write();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif
