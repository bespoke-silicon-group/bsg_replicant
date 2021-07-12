#include <bsg_manycore_dpi_tile.hpp>

// This method executes requests to dmem, icache, and csr-space like
// any normal tile.
void BsgDpiTile::execute_request(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp){
        this->default_request_handler(req, rsp);
}


// This is the traffic generator method. This particular test sends stats packets
// for all tag x start/end and kernel x start/end combinations
void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        // The iteration counter, iter, is set to 0 by the host during initialization
        uint32_t &iter = dmem_p[0];

        // in this particular test... always fences
        if(fence())
                return;

        if(iter == 0){
                *req_v_o = get_packet_stat_kernel_start(req_o);
                iter ++;
                return;
        }

        if((iter >= 1) && (iter <= 16)){
                *req_v_o = get_packet_stat_tag_start(req_o, iter - 1);
                iter ++;
                return;
        }


        if((iter >= 17) && (iter <= 32)){
                *req_v_o = get_packet_stat_tag_end(req_o, iter - 17);
                iter ++;
                return;
        }
        
        if(iter == 33){
                *req_v_o = get_packet_stat_kernel_end(req_o);
                iter ++;
                return;
        }

        *req_v_o = get_packet_finish(req_o);

        // Setting finished to true means this method will no longer
        // be called on this tile.
        finished = true;
}

void BsgDpiTile::receive_response(const hb_mc_response_packet_t *rsp_i){
        // Do nothing. Internally, BsgDpiTile will track and free IDs.
}
