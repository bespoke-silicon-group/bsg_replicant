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

#include "test_coordinate.hpp"
#include "bsg_manycore_coordinate.h"
#include "bsg_manycore_printing.h"
#include <inttypes.h>
#include <vector>

static int iterate(hb_mc_coordinate_t origin, hb_mc_dimension_t dimension, const std::vector<hb_mc_coordinate_t> &expect)
{
    hb_mc_coordinate_t now;
    int good = 1;
    int i = 0;
    char buffer[256];

    foreach_coordinate(now, origin, dimension)
    {
        good = good && hb_mc_coordinate_eq(now, expect[i]);
        bsg_pr_info("iteration %2d: %s\n", i, hb_mc_coordinate_to_string(now, buffer, sizeof(buffer)));
        i++;
    }

    return good && (i == expect.size());
}


static int iterate_x_y(hb_mc_coordinate_t origin, hb_mc_dimension_t dimension, const std::vector<hb_mc_coordinate_t> &expect)
{
    int good = 1;
    int i = 0;
    char buffer[256];
    hb_mc_idx_t x,y;

    foreach_x_y(x, y, origin, dimension)
    {
        hb_mc_coordinate_t now = HB_MC_COORDINATE(x,y);
        good = good && hb_mc_coordinate_eq(now, expect[i]);
        bsg_pr_info("iteration %2d: %s\n", i, hb_mc_coordinate_to_string(now, buffer, sizeof(buffer)));
        i++;
    }

    return good && (i == expect.size());
}



#define TEST(origin, dim, expect)                               \
    do {                                                        \
        bsg_pr_info("Starting test foreach_coordinate\n");      \
        int r = iterate(origin, dim, expect);                   \
        if (!r) return HB_MC_FAIL;                      \
        bsg_pr_info("Starting test foreach_x_y\n");     \
        r = iterate_x_y(origin, dim, expect);           \
        if (!r) return HB_MC_FAIL;                      \
    } while (0)

int test_coordinate (int argc, char **argv) {
    hb_mc_coordinate_t origin;
    hb_mc_dimension_t dim;
    std::vector<hb_mc_coordinate_t> expect;

    origin = HB_MC_COORDINATE(0,0);

    dim    = HB_MC_DIMENSION(1,1);
    expect = { HB_MC_COORDINATE(0,0) };
    TEST(origin, dim, expect);


    dim    = HB_MC_DIMENSION(2,1);
    expect = {
        HB_MC_COORDINATE(0,0),
        HB_MC_COORDINATE(1,0),
    };
    TEST(origin, dim, expect);

    dim = HB_MC_DIMENSION(1,2);
    expect = {
        HB_MC_COORDINATE(0,0),
        HB_MC_COORDINATE(0,1),
    };
    TEST(origin, dim, expect);

    origin = HB_MC_COORDINATE(0,2);
    dim    = HB_MC_DIMENSION(2,2);
    expect = {
        HB_MC_COORDINATE(0,2),
        HB_MC_COORDINATE(0,3),
        HB_MC_COORDINATE(1,2),
        HB_MC_COORDINATE(1,3),
    };
    TEST(origin, dim, expect);

    return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info("test_rom Regression Test \n");
        int rc = test_rom(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
