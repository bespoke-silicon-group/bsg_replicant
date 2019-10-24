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
#include <bsg_manycore_errno.h>
#include <list>
#include <stdint.h>

typedef std::list<hb_mc_responder_t *> responder_list;

static responder_list *responders = nullptr;

int hb_mc_responder_init(hb_mc_responder_t *responder, hb_mc_manycore_t *mc)
{
        int err;
        if (responder->init == nullptr)
                return HB_MC_INVALID; // no init

        err = responder->init(responder, mc);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}

int hb_mc_responders_init(hb_mc_manycore_t *mc)
{
        if (responders == nullptr)
                return HB_MC_SUCCESS; //  no responders

        for (auto it = responders->begin();
             it != responders->end();
             it++) {
                auto responder = *it;
                int err = hb_mc_responder_init(responder, mc);
                if (err != HB_MC_SUCCESS)
                        return err;
        }

        return HB_MC_SUCCESS;
}

int hb_mc_responder_quit(hb_mc_responder_t *responder, hb_mc_manycore_t *mc)
{
        int err;
        if (responder->quit == nullptr) // no quit
                return HB_MC_INVALID;

        err = responder->quit(responder, mc);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}

int hb_mc_responders_quit(hb_mc_manycore_t *mc)
{
        int err;

        if (responders == nullptr)
                return HB_MC_SUCCESS; // no responders

        for (auto it = responders->begin();
             it != responders->end();
             it++) {
                auto responder = *it;
                err = hb_mc_responder_quit(responder, mc);
                if (err != HB_MC_SUCCESS)
                        return err;
        }

        return HB_MC_SUCCESS;
}

static int hb_mc_responder_respond(hb_mc_responder_t *responder,
                                   hb_mc_manycore_t *mc,
                                   const hb_mc_request_packet_t *rqst)
{
        int err;

        if (responder->respond == nullptr)
                return HB_MC_INVALID; // no respond

        if (responder->ids == nullptr)
                return HB_MC_SUCCESS; // no ids

        for (hb_mc_request_packet_id_t *id = responder->ids;
             id->init != 0;
             id++) {
                if (hb_mc_request_packet_is_match(rqst, id) == 1)
                        return responder->respond(responder, mc, rqst);
        }

        return HB_MC_SUCCESS;
}

int hb_mc_responders_respond(hb_mc_manycore_t *mc, const hb_mc_request_packet_t *rqst)
{
        int err;

        if (responders == nullptr)
                return HB_MC_SUCCESS; // no responders

        for (auto it = responders->begin();
             it != responders->end();
             it++) {
                auto responder = *it;
                err = hb_mc_responder_respond(responder, mc, rqst);
                if (err != HB_MC_SUCCESS)
                        return err;
        }
        return HB_MC_SUCCESS;
}


int hb_mc_responder_add(hb_mc_responder_t *responder)
{
        if (responders == nullptr)
                responders = new responder_list;

        responders->push_front(responder);
        return HB_MC_SUCCESS;
}

int hb_mc_responder_del(hb_mc_responder_t *responder)
{
        if (responders == nullptr)
                return HB_MC_FAIL;

        responders->remove(responder);
        return HB_MC_SUCCESS;
}
