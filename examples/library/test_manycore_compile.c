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

#include "test_manycore_compile.h"

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

static
void print_config(hb_mc_manycore_t *manycore)
{
        bsg_pr_test_info("Printing configuration for '%s'\n", manycore->name);
        bsg_pr_test_info("%s: basejump-stl: 0x%" PRIx32 "\n",
                         manycore->name, hb_mc_config_get_githash_basejump(&manycore->config));
        bsg_pr_test_info("%s: manycore:     0x%" PRIx32 "\n",
                         manycore->name, hb_mc_config_get_githash_manycore(&manycore->config));
        bsg_pr_test_info("%s: f1:           0x%" PRIx32 "\n",
                         manycore->name, hb_mc_config_get_githash_f1(&manycore->config));
}

static
int test_manycore_compile(void)
{
        int rc;
        hb_mc_manycore_t manycore;
        rc = hb_mc_manycore_init(&manycore, "manycore@test_manycore_compile", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device!\n");
                return HB_MC_FAIL;
        }

        uintptr_t addr = hb_mc_mmio_fifo_get_reg_addr(HB_MC_FIFO_RX_REQ,
                                                      HB_MC_MMIO_FIFO_ISR_OFFSET);
        uint32_t val;
        hb_mc_manycore_mmio_read32(&manycore, addr, &val);
        bsg_pr_test_info("Value @ 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", addr, val);

        print_config(&manycore);
        return 0;
}
#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_manycore_compile Regression Test \n");
        int rc = test_manycore_compile();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
