#include <bsg_manycore_responder.h>
#include <bsg_manycore_request_packet_id.h>
#include <bsg_manycore_printing.h>
#include <stdio.h>

#define UART_EPA 0xEADC

static hb_mc_request_packet_id_t ids [] = {
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(0xEADC) ),
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

static int respond(hb_mc_responder_t *responder,
		   hb_mc_manycore_t *mc,
		   const hb_mc_request_packet_t *rqst)
{
	auto data = hb_mc_request_packet_get_data(rqst);
	FILE *f = (FILE*)responder->responder_data;
	fputc((int)data, f);
        return 0;
}

static hb_mc_responder_t uart_responder("UART", ids, init, quit, respond);

source_responder(uart_responder)
