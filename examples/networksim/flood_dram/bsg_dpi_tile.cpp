#include <bsg_manycore_dpi_tile.hpp>

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

// Instead, store iteration variables in DMEM.
int idx = 0; // Only used by origin
void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        // The host writes these values before unfreezing the tile.
        hb_mc_eva_t &base = dmem_p[1];
        uint32_t &nels = dmem_p[2];
        // ptr is our iteration variable. It is set by the host.
        hb_mc_eva_t &ptr = dmem_p[3];
        hb_mc_coordinate_t tg_dim = {.x = dmem_p[4], .y = dmem_p[5]};

        if(wait_at_barrier(0, tg_dim))
                return;

        if(is_origin() && idx == 0){
                *req_v_o = get_packet_stat_kernel_start(req_o);
                idx ++;
                return;
        }

        // This if statement is effectively a loop, since the method
        // is called on every cycle. Returning is effectively a
        // python-esque yield statement.
        hb_mc_eva_t limit = base + nels * sizeof(uint32_t);
        if(ptr < limit){
                *req_v_o = get_packet_from_eva<uint32_t>(req_o, ptr);
                // You MUST increment before returning.
                ptr += sizeof(uint32_t);
                return;
        }

        // Wait until all requests have returned, so that the data is
        // accumulated. Do not send packets while we are fencing.
        if(fence())
                return;

        if(wait_at_barrier(1, tg_dim))
                return;

        if(is_origin() && idx == 1){
                *req_v_o = get_packet_stat_kernel_start(req_o);
                idx ++;
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
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);

        dmem_p[dmem_sz/sizeof(*dmem_p) - 1] += rsp_i->data;
}
