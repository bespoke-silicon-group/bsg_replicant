#include <bsg_manycore_responder.h>
#include <bsg_manycore_request_packet_id.h>
#include <bsg_manycore_printing.h>
#include <stdio.h>

enum hb_mc_trace_epa_indx {
  BRANCH_TRACE_EPA_INDX, 

  HB_MC_NUM_TRACE_EPAS
};

typedef struct {
  FILE* f;
  const char* type;
} trace_config_t;

static hb_mc_request_packet_id_t ids [] = {
  [BRANCH_TRACE_EPA_INDX] = RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(0xEEE4) ),
  { /* sentinel */ },
};

static trace_config_t trace_config [] = {
  {.f = stderr, .type = "branch"},
};

static int init(hb_mc_responder_t *responder,
    hb_mc_manycore_t *mc)
{
  bsg_pr_dbg("hello from %s\n", __FILE__);
  responder->responder_data = trace_config;
  return 0;
}

static int quit(hb_mc_responder_t *responder,
    hb_mc_manycore_t *mc)
{
  bsg_pr_dbg("goodbye from %s\n", __FILE__);
  responder->responder_data = nullptr;
  return 0;
}

static int respond(hb_mc_responder_t *responder,
       hb_mc_manycore_t *mc,
       const hb_mc_request_packet_t *rqst)
{
  auto data = hb_mc_request_packet_get_data(rqst);
  auto src_x = hb_mc_request_packet_get_x_src(rqst);
  auto src_y = hb_mc_request_packet_get_y_src(rqst);

  for(int i=0; i<HB_MC_NUM_TRACE_EPAS; i++) {
    if(hb_mc_request_packet_is_match(rqst, &responder->ids[i])) {
      trace_config_t config = ((trace_config_t*) responder->responder_data)[i];
      fprintf(config.f, 
              "hbmc_%s_trace x=%d y=%d data=%x\n", 
              config.type, src_x, src_y, (int)data);
      break;
    }
  }

  return 0;
}

static hb_mc_responder_t trace_responder("TRACE", ids, init, quit, respond);

source_responder(trace_responder)
