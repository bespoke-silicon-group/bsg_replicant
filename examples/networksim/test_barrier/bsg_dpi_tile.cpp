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
        hb_mc_coordinate_t tg_dim = {.x = dmem_p[0], .y = dmem_p[1]};

        // wait_at_barrier provides barrier functionality.
        // wait_at_barrier takes a barrier_id as an integer. The
        // origin tile will verify that all tiles have reached the
        // barrier with the same barrier_id before "unblocking" or
        // causing wait_at_barrier to return false. Subsequent calls
        // to wait_at_barrier with the same id will return false.
        //
        // barrier_id must increase by 1 between call sites.
        if(wait_at_barrier(0, tg_dim))
                return;

        if(wait_at_barrier(1, tg_dim))
                return;

        if(wait_at_barrier(2, tg_dim))
                return;

        *req_v_o = get_packet_finish(req_o);

        // Setting finished to true means this method will no longer
        // be called.
        finished = true;
}

void BsgDpiTile::receive_response(const hb_mc_response_packet_t *rsp_i){
        // Do nothing. Internally, BsgDpiTile will track and free IDs.
}
