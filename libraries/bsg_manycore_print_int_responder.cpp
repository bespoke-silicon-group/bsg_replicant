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

#include <bsg_manycore_responder.h>
#include <bsg_manycore_request_packet_id.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_coordinate.h>
#include <stdio.h>

#define SINT_EPA 0xEAE0
#define UINT_EPA 0xEAE4
#define XINT_EPA 0xEAE8
#define FP32_EPA 0xEAEC
#define FP32_SCI_EPA 0xEAF0

static hb_mc_request_packet_id_t ids [] = {
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(SINT_EPA) ),
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(UINT_EPA) ),
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(XINT_EPA) ),
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(FP32_EPA) ),
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(FP32_SCI_EPA) ),
        { /* sentinel */ },
};

static int init(hb_mc_responder_t *responder,
                hb_mc_manycore_t *mc)
{
        bsg_pr_dbg("hello from %s\n", __FILE__);
        responder->responder_data = stdout;
        return 0;
}

static int quit(hb_mc_responder_t *responder,
                hb_mc_manycore_t *mc)
{
        bsg_pr_dbg("goodbye from %s\n", __FILE__);
        responder->responder_data = nullptr;
        return 0;
}

typedef union { float f; unsigned u; } utof_t;

static int respond(hb_mc_responder_t *responder,
                   hb_mc_manycore_t *mc,
                   const hb_mc_request_packet_t *rqst)
{
        auto data = hb_mc_request_packet_get_data(rqst);
        FILE *f = (FILE*)responder->responder_data;
        char coordstr[256];
        hb_mc_coordinate_to_string(hb_mc_coordinate(hb_mc_request_packet_get_x_src(rqst),
                                                    hb_mc_request_packet_get_y_src(rqst)),
                                   coordstr,sizeof(coordstr));
        utof_t f_data;

        switch (hb_mc_request_packet_get_epa(rqst)) {
        case SINT_EPA:
                fprintf(f, "int32   from %s: %d\n", coordstr, (int)data);
                break;
        case UINT_EPA:
                fprintf(f, "uint32  from %s: %u\n", coordstr, (unsigned)data);
                break;
        case XINT_EPA:
                fprintf(f, "uint32  from %s: 0x%08x\n", coordstr, (unsigned) data);
                break;
        case FP32_EPA:
                f_data.u = data;
                fprintf(f, "float32 from %s: %f\n", coordstr, f_data.f);
                break;
        case FP32_SCI_EPA:
                f_data.u = data;
                fprintf(f, "float32 from %s: %e\n", coordstr, f_data.f);
                break;
        }
        fflush(f);
        return 0;
}

static hb_mc_responder_t print_int_responder("Print Int", ids, init, quit, respond);

source_responder(print_int_responder)
