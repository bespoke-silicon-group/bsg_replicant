#include "bsg_manycore_regression.h"
#include "bsg_manycore_cuda.h"
#include "bsg_manycore.h"
#include "bsg_manycore_responder.h"
#include <cstring>
#include <vector>

static hb_mc_device_t *dev = nullptr;
static std::vector<int> pkt_data;


//////////////////////////////////////////////////////
// Responder to check for packets from the manycore //
//////////////////////////////////////////////////////
static
hb_mc_request_packet_id_t resp_ids [] = {
    RQST_ID(RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(0x8888)),
    {/*sentinal*/},
};

static int resp_init(hb_mc_responder_t *resp, hb_mc_manycore_t *mc)
{
    return HB_MC_SUCCESS;
}

static int resp_quit(hb_mc_responder_t *resp, hb_mc_manycore_t *mc)
{
    return HB_MC_SUCCESS;
}

static int resp_respond(hb_mc_responder_t *resp, hb_mc_manycore_t *mc, const hb_mc_request_packet_t *rqst)
{
    bsg_pr_dbg("%s: received packet from (%3d,%3d)\n"
               , __func__
               , rqst->x_src
               , rqst->y_src);

    pkt_data.push_back(static_cast<int>(hb_mc_request_packet_get_data(rqst)));
    return HB_MC_SUCCESS;
}


static
hb_mc_responder_t resp ("barrier-test", resp_ids, resp_init, resp_quit, resp_respond);
source_responder(resp);

////////////////////////
// Main hosst program //
////////////////////////
int barrier_test_main(int argc, char *argv[])
{
    char *rvp    = argv[1];
    char *kname  = argv[2];
    int tgx = atoi(argv[3]);
    int tgy = atoi(argv[4]);

    dev = (hb_mc_device_t*)malloc(sizeof(*dev));
    if (!dev) {
        bsg_pr_err("failed to allocate memory\n");
        return HB_MC_NOMEM;
    }

    // initialize
    BSG_CUDA_CALL(hb_mc_device_init(dev, "cuda hw barrier", DEVICE_ID));
    BSG_CUDA_CALL(hb_mc_device_program_init(dev, rvp, "baralloc", 0));

    // enque job and execute
    hb_mc_dimension_t gd, tgd;
    gd = hb_mc_dimension(1,1);
    tgd = hb_mc_dimension(tgx, tgy);

    BSG_CUDA_CALL(hb_mc_kernel_enqueue(dev, gd, tgd, kname, 0, nullptr));
    BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(dev));

    // cleanup
    BSG_CUDA_CALL(hb_mc_device_program_finish(dev));
    BSG_CUDA_CALL(hb_mc_device_finish(dev));

    // check that we got the right number of packets
    if (static_cast<int>(pkt_data.size()) != tgx*tgy) {
        bsg_pr_err("Expected %d packets from %3d X %3d group, received %d\n"
                   , tgx*tgy
                   , tgx
                   , tgy
                   , static_cast<int>(pkt_data.size()));
        return HB_MC_FAIL;
    }

    // validate results by checking that the expected packets arrived in-order
    int id = 0;
    for (int x = 0; x < tgx; x++) {
        for (int y = 0; y < tgy; y++) {
            int data = pkt_data[id];
            int dx = (data >> 16) & 0xffff;
            int dy = data & 0xffff;

            if (x != dx || y != dy) {
                bsg_pr_err("Packet %d: expected from (%d,%d) but found (%d,%d)\n"
                           , id
                           , x
                           , y
                           , dx
                           , dy);
                return HB_MC_FAIL;
            }
            id++;
        }
    }
    return HB_MC_SUCCESS;
}

declare_program_main("HW Barrier Test", barrier_test_main);

