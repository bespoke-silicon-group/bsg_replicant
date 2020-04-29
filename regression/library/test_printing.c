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

/********************************************************************/
/* This tests the print macros defined in `bsg_manycore_printing.h` */
/* The expected output is:                                          */
/*                                                                  */
/* DEBUG:   testing 1... 2... 3...                                  */
/* DEBUG:   hello                                                   */
/* DEBUG:   from                                                    */
/* DEBUG:   test_printing                                           */
/* DEBUG:                                                           */
/* DEBUG:   done                                                    */
/* ERROR:   testing error                                           */
/* WARNING: 1                                                       */
/* WARNING: 2                                                       */
/* WARNING: hello                                                   */
/* INFO:    hello from                                              */
/* INFO:    info                                                    */
/********************************************************************/

#define DEBUG
#include "test_printing.h"
static int test_printing(void)
{
        int rc;
        hb_mc_manycore_t mc = {0};
        rc = hb_mc_manycore_init(&mc, "manycore@test_rom", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device: %s\n",
                                hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        bsg_pr_dbg("testing ");
        bsg_pr_dbg("1... ");
        bsg_pr_dbg("2... ");
        bsg_pr_dbg("3... ");
        bsg_pr_dbg("\n");
        
        bsg_pr_dbg("hello\nfrom\n%s\n", __func__);
        bsg_pr_dbg("\ndone\n");

        bsg_pr_err("testing error");
        bsg_pr_err("\n");

        bsg_pr_warn("%d\n%d\n%s\n", 1, 2, "hello");

        bsg_pr_info("%s %s\n%s\n", "hello", "from", "info");
        rc = hb_mc_manycore_exit(&mc);

        return 0;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_printing Regression Test\n");
        int rc = test_printing();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
