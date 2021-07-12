#include <bsg_manycore_dpi_tile.hpp>

// This method executes requests to dmem, icache, and csr-space like
// any normal tile.
void BsgDpiTile::execute_request(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp){
        this->default_request_handler(req, rsp);
}


// This is the traffic generator method. This particular test sends a
// finish packet when dmem address 4 changes from 0 to 1 (The second
// DMEM write in the example above)
void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        uint32_t &phase = dmem_p[1];

        if(phase == 0){
                *req_v_o = get_packet_finish(req_o);
                phase ++;
                return;
        }

        // Setting finished to true means this method will no longer
        // be called.
        if(fence())
                return;

        finished = true;
}

void BsgDpiTile::receive_response(const hb_mc_response_packet_t *rsp_i){
        // Do nothing. Internally, BsgDpiTile will track and free IDs.
}

