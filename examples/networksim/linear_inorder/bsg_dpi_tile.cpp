#include <bsg_manycore_dpi_tile.hpp>
#include <alloc.hpp>

// This method executes requests to dmem, icache, and csr-space like
// any normal tile.
void BsgDpiTile::execute_request(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp){
        this->default_request_handler(req, rsp);
}


// This is the traffic generator method. It will not be called until
// the tile is unfrozen, and then it will be called on every cycle
// that a packet can be sent.

// WARNING: Do not use static variables for iteration, they
// are shared between all instances of a class (unless the
// templates are different)

void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        // The host writes these values before unfreezing the tile.
        hb_mc_eva_t &base = dmem_p[0];
        uint32_t &nels = dmem_p[1];
        uint32_t &offset = dmem_p[2];
        uint32_t &stride = dmem_p[3];
        // iter is our iteration variable. It is set by the host.
        uint32_t &iter = dmem_p[4];
        uint32_t &limit = dmem_p[5];
        uint32_t &phase = dmem_p[6];
        hb_mc_coordinate_t tg_dim = {.x = dmem_p[7], .y = dmem_p[8]};

        // Wait for everyone to start
        if(wait_at_barrier(0, tg_dim))
                return;


        // Start stats for the vcache (but only send from origin)
        if(phase == 0){
                set_credit_limit(get_credit_alloc(me.y-origin.y, me.x-origin.x));
                if(is_origin()){
                        *req_v_o = get_packet_stat_kernel_start(req_o);
                }
                phase ++;
                return;
        }

        // Start stats for the tiles. Does not send a packet, but will
        // print lines to dpi_stats.csv
        if(phase == 1){
                if(fence())
                        return;
                get_packet_stat_tag_start(req_o, 0);
                phase ++;
        }

        if(wait_at_barrier(1, tg_dim))
                return;

        // This if statement is effectively a loop, since the method
        // is called on every cycle. Returning is effectively a
        // python-esque yield statement.
        if(iter < limit){
                // Every 32 loads, fence until all reads return.
                //if((iter % 32) == 0 && fence_read())
                //        return;
                *req_v_o = get_packet_from_eva<uint32_t>(req_o, base + ((iter*stride + offset) % nels) * sizeof(uint32_t));
                
                // Make each response take 1 cycle to
                // process. Responses are processed in order. This
                // emulates a 32-load, 32-store instruction pattern.
                set_packet_rx_cost(req_o, 1);

                // You MUST increment before returning.
                iter ++;
                return;
        }

        // Wait until all requests have returned, so that the data is
        // accumulated. Do not send packets while we are fencing.
        if(fence_read())
                return;

        // Cause stats to be printed and record finish time
        if(phase == 2){
                get_packet_stat_tag_end(req_o, 0);
                phase ++;
        }

        if(wait_at_barrier(2, tg_dim))
                return;

        if(phase == 3){
                // Print the stats
                if(is_origin())
                        *req_v_o = get_packet_stat_kernel_end(req_o);
                phase ++;
                return;
        }

        // Send the finish packet, once
        *req_v_o = get_packet_finish(req_o);

        // Setting finished to true means this method will no longer
        // be called.
        finished = true;
        return;
}

void BsgDpiTile::receive_response(const hb_mc_response_packet_t *rsp_i){
        float *dmem_p = reinterpret_cast<float *>(this->dmem);
        float val = *reinterpret_cast<const float *>(&rsp_i->data);

        dmem_p[dmem_sz/sizeof(*dmem_p) - 1] = dmem_p[dmem_sz/sizeof(*dmem_p) - 1] / val;
}
