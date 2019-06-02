#include <bsg_manycore_request_packet_id.h>

#define UART_EPA 0xEADC

static hb_mc_request_packet_id_t ids [] = {
        RQST_ID( RQST_ID_ANY_X, RQST_ID_ANY_Y, RQST_ID_ADDR(0xEADC) ),
        RQST_ID( RQST_ID_RANGE_X(1,3), RQST_ID_ANY_Y, RQST_ID_ADDR(0xEAD8) ),
        RQST_ID( RQST_ID_RANGE_X(1,3), RQST_ID_RANGE_Y(1,2), RQST_ID_ADDR(0xEAD8) ),
        { /* sentinel */ },
};
