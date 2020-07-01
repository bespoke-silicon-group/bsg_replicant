// Copyright (c) 2020, University of Washington All rights reserved.
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

// This file implements the functions that are specific to the aws-vcs
// platform. The majority of the platform-specific functions are
// reused by linking with the platform implmentation in aws-fpga

#include <bsg_manycore_platform.h>
#include <fpga_pci_sv.h>
#include <svdpi.h>
#include <utils/sh_dpi_tasks.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void sv_set_virtual_dip_switch(int, int);

#ifdef __cplusplus
}
#endif

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * This exists to hide the DPI clock-switching hack we use to
 * reduce the runtime in VCS/F1 Simulation. Most
 * implementations can return HB_MC_SUCCESS.
 *
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc){
        sv_set_virtual_dip_switch(0, 1);

        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * This exists to hide the DPI clock-switching hack we use to
 * reduce the runtime in VCS/F1 Simulation. Most
 * implementations can return HB_MC_SUCCESS.
 *
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc){
        sv_set_virtual_dip_switch(0, 0);

        return HB_MC_SUCCESS;
}
