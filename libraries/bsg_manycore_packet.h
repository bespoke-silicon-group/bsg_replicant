#ifndef BSG_MANYCORE_PACKET_H
#define BSG_MANYCORE_PACKET_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_request_packet.h>
#include <bsg_manycore_response_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union packet {
	hb_mc_request_packet_t request; /**/
	hb_mc_response_packet_t response; /* from the Hammerblade Manycore */
	uint32_t words[4];
} hb_mc_packet_t;

#ifdef __cplusplus
}
#endif
#endif
