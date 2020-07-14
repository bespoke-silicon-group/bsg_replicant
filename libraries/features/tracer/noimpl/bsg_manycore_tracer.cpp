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
#include <bsg_manycore_tracer.hpp>
#include <bsg_manycore_printing.h>

/**
 * Initialize an hb_mc_tracer_t instance
 * @param[in] p    A pointer to the hb_mc_tracer_t instance to initialize
 * @param[in] hier An implementation-dependent string. See the implementation for more details.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * NOTE: In this implementation, the argument hier indicates the path
 * to the top level module in simulation.
 */
int hb_mc_tracer_init(hb_mc_tracer_t *p, string &hier){
        return HB_MC_NOIMPL;
}

/**
 * Clean up an hb_mc_tracer_t instance
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_tracer_cleanup(hb_mc_tracer_t *p){
        return HB_MC_NOIMPL;
}

/**
 * Enable trace file generation (vanilla_operation_trace.csv)
 * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_tracer_trace_enable(hb_mc_tracer_t p){
        bsg_pr_warn("%s: Not supported.\n", __func__);
        return HB_MC_NOIMPL;
}

/**
 * Disable trace file generation (vanilla_operation_trace.csv)
 * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_tracer_trace_disable(hb_mc_tracer_t p){
        bsg_pr_warn("%s: Not supported.\n", __func__);
        return HB_MC_NOIMPL;
}

/**
 * Enable log file generation (vanilla.log)
 * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_tracer_log_enable(hb_mc_tracer_t p){
        bsg_pr_warn("%s: Not supported.\n", __func__);
        return HB_MC_NOIMPL;
}

/**
 * Disable log file generation (vanilla.log)
 * @param[in] p    A tracer instance initialized with hb_mc_tracer_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_tracer_log_disable(hb_mc_tracer_t p){
        bsg_pr_warn("%s: Not supported.\n", __func__);
        return HB_MC_NOIMPL;
}
int hb_mc_profiler_init(hb_mc_profiler_t *p, hb_mc_idx_t x, hb_mc_idx_t y, string &hier){
        return HB_MC_NOIMPL;
}

/**
 * Clean up an hb_mc_profiler_t instance
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_profiler_cleanup(hb_mc_profiler_t *p){
        return HB_MC_NOIMPL;
}

/**
 * Get the number of instructions executed for a certain class of instructions
 * @param[in] p     A hb_mc_profiler_t instance initialized with hb_mc_profiler_init()
 * @param[in] itype An enum defining the class of instructions to query.
 * @param[out] count The number of instructions executed in the queried class.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_profiler_get_icount(hb_mc_profiler_t p, bsg_instr_type_e itype, int *count){
        bsg_pr_warn("%s: Not supported.\n", __func__);
        return HB_MC_NOIMPL;
}
