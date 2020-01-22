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

#include <algorithm>
#include <type_traits>
#include <bsg_manycore_npa.h>
#include "test_vcache_flush.hpp"

#define NUM_TESTS 32

/* these are read from the designs ROM */
uint32_t NUM_SETS = 0;
uint32_t ASSOCIATIVITY = 0;
uint32_t CACHE_LINE_SIZE_WORDS = 0;
uint32_t CACHE_LINE_SIZE = 0;
uint32_t ADDR_MASK = 0;

#define ALIGN_4_MASK ~0x3

int flush_cache_line(hb_mc_manycore_t *mc, hb_mc_epa_t addr, hb_mc_idx_t x, hb_mc_idx_t y)
{
        int rc;
        hb_mc_npa_t npa;
        hb_mc_epa_t flush_addr;
        hb_mc_coordinate_t dest = hb_mc_coordinate(x,y);
        uint64_t stride;
        uint32_t result;

        for(int i = 1; i <= ASSOCIATIVITY; i++)
        {
                stride = i * NUM_SETS * CACHE_LINE_SIZE;
                flush_addr = (hb_mc_epa_t)(((uint64_t)addr + stride) & ADDR_MASK);
                npa = hb_mc_epa_to_npa(dest, flush_addr);

                rc = hb_mc_manycore_read32(mc, &npa, &result);
                bsg_pr_test_info("%s -- Sending read command to address 0x%08x at tile"
                                " (%d, %d).\n", __func__, flush_addr, x, y);
                if(rc != HB_MC_SUCCESS) {
                        bsg_pr_test_err("%s -- hb_mc_read32 failed!\n", __func__);
                        return HB_MC_FAIL;
                }
        }
        return HB_MC_SUCCESS;
}

template <typename T>
int test_address(hb_mc_manycore_t *mc, hb_mc_epa_t addr, T data, hb_mc_idx_t x, hb_mc_idx_t y) {

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
        constexpr int bit_size = sizeof(T) * 8;
        
        int rc;
        hb_mc_coordinate_t dest = hb_mc_coordinate(x,y);
        hb_mc_npa_t npa;
        T result;

        bsg_pr_test_info("Testing Address 0x%08x at (%d, %d) with data: 0x%08x\n",
                        addr, x, y, data);

        bsg_pr_test_info("Sending write command to address 0x%08x at tile (%d, %d)\n",
                        addr, x, y);

        npa = hb_mc_epa_to_npa(dest, addr);
        rc = write_fn(mc, &npa, data);
        if(rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("%s -- hb_mc_write%d failed!\n", bit_size, __func__);
                return HB_MC_FAIL;
        }

        bsg_pr_test_info("%s -- Sending read command to address 0x%08x at tile"
                        " (%d, %d). (It should be in VCache.)\n",
                        __func__, addr, x, y);
        rc = read_fn(mc, &npa, &result);
        if(rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("%s -- hb_mc_read%d failed!\n", bit_size, __func__);
                return HB_MC_FAIL;
        }

        if(result != data) {
                bsg_pr_test_err("%s -- Incorrect data read from Victim Cache!. "
                                "Got 0x%08x, but expected 0x%08x\n",
                                __func__, result, data);
                return HB_MC_FAIL;
        }

        
        bsg_pr_test_info("%s -- Flushing cache line\n", __func__);
        rc = flush_cache_line(mc, addr, x, y);
        if(rc != HB_MC_SUCCESS)
                return HB_MC_FAIL;

        bsg_pr_test_info("%s -- Sending read command to address 0x%08x at tile"
                        " (%d, %d). (It should be in DRAM.)\n",
                        __func__, addr, x, y);
        
        result = ~result;
        rc = read_fn(mc, &npa, &result);
        if(rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("%s -- hb_mc_read%d failed!\n", bit_size, __func__);
                return HB_MC_FAIL;
        }

        if(result != data) {
                bsg_pr_test_err("%s -- Incorrect data read from DRAM!. "
                                "Got 0x%08x, but expected 0x%08x\n",
                                __func__, result, data);
                return HB_MC_FAIL;
        }

        return HB_MC_SUCCESS;
}

int test_vcache_flush() {
        int rc, i;
        hb_mc_manycore_t mc = HB_MC_MANYCORE_INIT;
        const hb_mc_config_t *config;
        hb_mc_dimension_t dim;
        hb_mc_coordinate_t host, dest;
        hb_mc_idx_t host_x, host_y, dim_x, dim_y;

        uint32_t addr_bitwidth;
        hb_mc_epa_t addrs[NUM_TESTS];
        hb_mc_epa_t addr;
        srand(0xDEADBEEF);

        rc = hb_mc_manycore_init(&mc, "manycore@test_vcache_stride", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device!\n");
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);

        host = hb_mc_config_get_host_interface(config);
        host_x = hb_mc_coordinate_get_x(host);
        host_y = hb_mc_coordinate_get_y(host);

        dim = hb_mc_config_get_dimension_network(config);
        dim_x = hb_mc_dimension_get_x(dim);
        dim_y = hb_mc_dimension_get_y(dim);
        
        addr_bitwidth = hb_mc_config_get_network_bitwidth_addr(config);
        
        ASSOCIATIVITY = hb_mc_config_get_vcache_ways(config);
        NUM_SETS = hb_mc_config_get_vcache_sets(config);
        CACHE_LINE_SIZE_WORDS = hb_mc_config_get_vcache_block_words(config);
        CACHE_LINE_SIZE = hb_mc_config_get_vcache_size(config);
        ADDR_MASK = ((1 << addr_bitwidth) - 1) & ALIGN_4_MASK;

        int dram = 0, ndrams = dim_x;
        hb_mc_idx_t dram_coord_y = hb_mc_config_get_dram_y(config);
        hb_mc_idx_t dram_coord_x = -1;
        hb_mc_idx_t dram_xs[dim_x];
        size_t dram_size = hb_mc_config_get_dram_size(config);
        // set all dram_xs to their unique column id
        for (hb_mc_idx_t x = 0; x < dim_x; ++x) dram_xs[x] = x;

        addrs[0] = 0;
        // VCaches partition DRAM evently amongst themselves
        addrs[NUM_TESTS - 1] = std::min(static_cast<hb_mc_epa_t>((1 << addr_bitwidth) - 1),
                                        static_cast<hb_mc_epa_t>((dram_size / ndrams) - 1)) & ALIGN_4_MASK;

        // Generate some random addresses
        for(int i = 1; i < NUM_TESTS - 1; i++)
                addrs[i] = (rand() % addrs[NUM_TESTS - 1]) & ALIGN_4_MASK;

        for (dram = 0; dram < ndrams; ++dram) {
                dram_coord_x = dram_xs[dram];
                for(i = 0; i < NUM_TESTS; ++i) {
                        addr = addrs[i];

                        bsg_pr_test_info("Testing 32-bit\n");
                        rc = test_address(&mc, addr, static_cast<uint32_t>(rand()), dram_coord_x, dram_coord_y);

                        bsg_pr_test_info("Testing 16-bit\n");
                        rc = test_address(&mc, addr    , static_cast<uint16_t>(rand() & 0xFFFF), dram_coord_x, dram_coord_y);
                        rc = test_address(&mc, addr + 2, static_cast<uint16_t>(rand() & 0xFFFF), dram_coord_x, dram_coord_y);

                        bsg_pr_test_info("Testing  8-bit\n");
                        rc = test_address(&mc, addr    ,  static_cast<uint8_t>(rand() & 0xFF), dram_coord_x, dram_coord_y);
                        rc = test_address(&mc, addr + 1,  static_cast<uint8_t>(rand() & 0xFF), dram_coord_x, dram_coord_y);
                        rc = test_address(&mc, addr + 2,  static_cast<uint8_t>(rand() & 0xFF), dram_coord_x, dram_coord_y);
                        rc = test_address(&mc, addr + 3,  static_cast<uint8_t>(rand() & 0xFF), dram_coord_x, dram_coord_y);
                }
                if(rc != HB_MC_SUCCESS)
                        return rc;
        }
        return HB_MC_SUCCESS;
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
        bsg_pr_test_info("test_vcache_flush Regression Test (COSIMULATION)\n");
        int rc = test_vcache_flush();
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char ** argv) {
        bsg_pr_test_info("test_vcache_flush Regression Test (F1)\n");
        int rc = test_vcache_flush();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif
