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

#include <vanilla_core_profiler.hpp>
#include <bsg_manycore_profiler.hpp>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_config.h>
#include <string>
#include <sstream>
#include <vector>
using namespace bsg_nonsynth_dpi;
using namespace std;

/**
 * Initialize an hb_mc_profiler_t instance
 * @param[in] p    A pointer to the hb_mc_profiler_t instance to initialize
 * @param[in] hier An implementation-dependent string. See the implementation for more details.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * NOTE: In this implementation, the argument hier indicates the path
 * to the top level module in simulation.
 */
int hb_mc_profiler_init(hb_mc_profiler_t *p, hb_mc_idx_t x, hb_mc_idx_t y, string &hier){

        // The crossbar and the Mesh hierarchy differ, so we need to
        // figure out which one exists. There is no way to query the
        // DPI interface to see what scopes are available, so instead
        // we attempt to get the scope for the (1,0) tile profiler
        // using getScopeFromName and if that returns 0/NULL then the
        // scope does not exist.

        // We check for the crossbar hierarchy first, and then assume
        // mesh if that scope does not exist.

        // If instance names in the RTL hierarchy change for either
        // network topology, then the definition of the strings tail
        // and tile01 (below) will need to change to reflect the new
        // naming.
        string tile01 = ".y[1].x[0].proc.vcore.vcore_prof";
        string xbar = hier + tile01;
        string tail;
        svScope scope;
        scope = svGetScopeFromName(xbar.c_str());
        if(scope)
                tail = ".proc.vcore.vcore_prof";
        else
                tail = ".tile.proc.h.z.vcore.vcore_prof";

        // Each profiler is located in the RTL hierarchy at: hier +
        // y[y-index].x[x-index] + tail. We need to construct that
        // string in the for-loop below.

        // We construct a dpi_vanilla_core_profiler instance for each
        // profiler in the HDL, and track it using a vector.
        vector<dpi_vanilla_core_profiler* > *profilers = new vector<dpi_vanilla_core_profiler* >();
        
        // Construct the objects, and strings.
        for(int iy = HB_MC_CONFIG_VCORE_BASE_Y-1; iy <= y; ++iy){
                for(int ix = 0; ix < x; ++ix){
                        ostringstream stream;
                        stream << hier << ".y[" << iy << "]" << ".x[" << ix << "]" << tail;
                        profilers->push_back(new dpi_vanilla_core_profiler(stream.str()));
                }
        }
        
        // Save the 2D vector
        *p = reinterpret_cast<hb_mc_profiler_t>(profilers);
        return HB_MC_SUCCESS;
}

/**
 * Clean up an hb_mc_profiler_t instance
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_profiler_cleanup(hb_mc_profiler_t *p){
        vector<dpi_vanilla_core_profiler *> *profilers =
                reinterpret_cast<vector<dpi_vanilla_core_profiler *> *>(*p);
        dpi_vanilla_core_profiler * prof;
        // From last to first (reverse order) remove elements from the
        // vectors, and delete the associated bojects.
        while (!profilers->empty()){
                prof = profilers->back();
                delete prof;
                profilers->pop_back();
        }

        delete profilers;

        return HB_MC_SUCCESS;
}

/**
 * Get the number of instructions executed for a certain class of instructions
 * @param[in] p     A hb_mc_profiler_t instance initialized with hb_mc_profiler_init()
 * @param[in] itype An enum defining the class of instructions to query.
 * @param[out] count The number of instructions executed in the queried class.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_profiler_get_icount(hb_mc_profiler_t p, bsg_instr_type_e itype, int *count){
        int err;
        int sum = 0, cur;
        vector<dpi_vanilla_core_profiler *> *profilers =
                reinterpret_cast<vector<dpi_vanilla_core_profiler *> *>(p);

        for (auto it = profilers->begin() ; it != profilers->end(); ++it){
                err = (*it)->get_instr_count(itype, &cur);
                sum += cur;
                if(err != BSG_NONSYNTH_DPI_SUCCESS){
                        if(err == BSG_NONSYNTH_DPI_NOT_WINDOW)
                                bsg_pr_err("%s: Called while not in valid clock window. (is reset still high?)\n", __func__);
                        return HB_MC_FAIL;
                }
        }

        *count = sum;
        return HB_MC_SUCCESS;
}

