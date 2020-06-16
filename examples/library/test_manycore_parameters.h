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

#ifndef __TEST_MANYCORE_PARAMETERS_H
#define __TEST_MANYCORE_PARAMETERS_H

// These are explicitly not copied from the bsg_manycore_eva.h 
#define LOCAL_EPA_SIZE (1<<18)
#define DMEM_EPA_SIZE (1<<12)
#define DMEM_EPA_OFFSET (1<<12)
#define DMEM_EVA_OFFSET (1<<12)

#define GROUP_INDICATOR (1 << 29)
#define GROUP_EPA_SIZE DMEM_EPA_SIZE
#define GROUP_X_BITS 6
#define GROUP_X_MAX ((1 << GROUP_X_BITS) - 1)
#define GROUP_X_MASK GROUP_X_MAX
#define GROUP_X_OFFSET 18

#define GROUP_Y_BITS 5
#define GROUP_Y_MAX ((1 << GROUP_Y_BITS) - 1)
#define GROUP_Y_MASK GROUP_Y_MAX
#define GROUP_Y_OFFSET (GROUP_X_OFFSET + GROUP_X_BITS)

#define GLOBAL_INDICATOR_WIDTH 1
#define GLOBAL_INDICATOR (1 << 30)
#define GLOBAL_EPA_SIZE DMEM_EPA_SIZE
#define GLOBAL_X_BITS 6
#define GLOBAL_X_MAX ((1 << GLOBAL_X_BITS) - 1)
#define GLOBAL_X_MASK GLOBAL_X_MAX
#define GLOBAL_X_OFFSET 18

#define GLOBAL_Y_BITS 6
#define GLOBAL_Y_MAX ((1 << GLOBAL_Y_BITS) - 1)
#define GLOBAL_Y_MASK GLOBAL_Y_MAX
#define GLOBAL_Y_OFFSET (GLOBAL_X_OFFSET + GLOBAL_X_BITS)

#define DRAM_INDICATOR_WIDTH 1
#define DRAM_INDICATOR (1 << 31)
#define DRAM_EPA_SIZE 16384
#define DRAM_STRIPE_WIDTH 6
#define DRAM_STRIPE_MASK ((1 << DRAM_STRIPE_WIDTH) - 1)


#endif // __TEST_MANYCORE_PARAMETERS_H
