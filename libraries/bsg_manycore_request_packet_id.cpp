#include <bsg_manycore_request_packet.h>
#include <bsg_manycore_request_packet_id.h>
#include <bsg_manycore_coordinate.h>

static int x_is_match(const hb_mc_request_packet_t *pkt,
		      const hb_mc_request_packet_id_t *pktid)
{
	uint8_t x = hb_mc_request_packet_get_x_src(pkt);

	return (pktid->id_x_src.x_lo <= x) && (x <= pktid->id_x_src.x_hi);
}

static int y_is_match(const hb_mc_request_packet_t *pkt,
		      const hb_mc_request_packet_id_t *pktid)
{
	uint8_t y = hb_mc_request_packet_get_y_src(pkt);

	return (pktid->id_y_src.y_lo <= y) && (y <= pktid->id_y_src.y_hi);
}

static int addr_is_match(const hb_mc_request_packet_t *pkt,
			 const hb_mc_request_packet_id_t *pktid)
{
	hb_mc_epa_t epa = hb_mc_request_packet_get_epa(pkt);
	/* check for equivalency under mask */
	return (pktid->id_addr.a_mask & epa) == pktid->id_addr.a_value;
}

int hb_mc_request_packet_is_match(const hb_mc_request_packet_t    *pkt,
                                  const hb_mc_request_packet_id_t *pktid)
{
	return x_is_match(pkt, pktid) && y_is_match(pkt, pktid) && addr_is_match(pkt, pktid);
}
