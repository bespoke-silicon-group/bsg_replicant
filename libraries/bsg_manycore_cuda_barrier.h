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

#ifndef BSG_MANYCORE_CUDA_BARRIER_H
#define BSG_MANYCORE_CUDA_BARRIER_H
#include <bsg_manycore_features.h>
#include <bsg_manycore_config.h>
#ifdef __cplusplus
extern "C" {
#endif

static inline int hb_mc_hw_barrier_csr_val(const hb_mc_config_t *cfg, int x, int y, int tx, int ty)
{
    const int RUCHE_FACTOR_X = cfg->bar_ruche_factor.x;
    const int OUTDIR_OFFSET = 16;

    // center tile coordinate
    int center_x = (tx/2);
    int center_y = (ty/2);


    // tile id
    int id = x + (y*tx);

    // input P is always on.
    int val = 1;

    // setting output dir
    if (x <= center_x - RUCHE_FACTOR_X) {
            // output = RE
            val |= (6 << OUTDIR_OFFSET);
    } else if ((x < center_x) && (x > (center_x - RUCHE_FACTOR_X))) {
            // output = E
            val |= (2 << OUTDIR_OFFSET);
    } else if (x == center_x) {
            if (y < center_y) {
                    // output = S
                    val |= (4 << OUTDIR_OFFSET);
            } else if (y == center_y) {
                    // output = Root
                    val |= (7 << OUTDIR_OFFSET);
            } else {
                    // output = N
                    val |= (3 << OUTDIR_OFFSET);
            }
    } else if ((x > center_x) && (x < (center_x + RUCHE_FACTOR_X))) {
            // output = W
            val |= (1 << OUTDIR_OFFSET);
    } else {
            // output = RW
            val |= (5 << OUTDIR_OFFSET);
    }

    // setting input mask
    // input = W
    if (((x == (center_x-1)) || (x == center_x))  && x > 0) {
            val |= (1 << 1);
    }

    // input = RW
    if (((x - RUCHE_FACTOR_X) >= 0) && (x <= center_x)) {
            val |= (1 << 5);
    }

    // input = E
    if (((x == (center_x+1)) || (x == center_x)) && (x < (tx-1))) {
            val |= (1 << 2);
    }

    // input = RE
    if (((x+RUCHE_FACTOR_X) < tx) && (x >= center_x)) {
            val |= (1 << 6);
    }

    if (x == center_x) {
            // input = N
            if ((y > 0) && (y <= center_y)) {
                    val |= (1 << 3);
            }
            // input = S
            if ((y < (ty-1)) && (y >= center_y)) {
                    val |= (1 << 4);
            }
    }

    return val;
}


#ifdef __cplusplus
}
#endif
#endif
