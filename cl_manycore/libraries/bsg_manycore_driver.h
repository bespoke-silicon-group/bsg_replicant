#ifndef BSG_MANYCORE_DRIVER_H
#define BSG_MANYCORE_DRIVER_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_bits.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_packet.h>
#include <bsg_manycore_fifo.h>
#include <endian.h>

#ifdef __cplusplus
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#else
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COSIM
static char *hb_mc_mmap_ocl (uint8_t fd);
#endif

__attribute__((deprecated))
int hb_mc_fifo_transmit (uint8_t fd, hb_mc_fifo_tx_t type, const hb_mc_packet_t *packet);
__attribute__((deprecated))
int hb_mc_fifo_receive (uint8_t fd, hb_mc_fifo_rx_t type, hb_mc_packet_t *packet);

__attribute__((deprecated))
int hb_mc_get_host_credits (uint8_t fd);
__attribute__((deprecated))
int hb_mc_all_host_req_complete(uint8_t fd);

int hb_mc_get_config(uint8_t fd, hb_mc_config_id_t id, uint32_t *cfg);
__attribute__((deprecated))
uint8_t hb_mc_get_manycore_dimension_x();
__attribute__((deprecated))
uint8_t hb_mc_get_manycore_dimension_y();
__attribute__((deprecated))
uint8_t hb_mc_get_host_intf_coord_x();
__attribute__((deprecated))
uint8_t hb_mc_get_host_intf_coord_y();
__attribute__((deprecated))
int hb_mc_fifo_finish(uint8_t fd);
__attribute__((deprecated))
int hb_mc_fifo_init(uint8_t *fd);
__attribute__((deprecated))
int hb_mc_fifo_check(uint8_t fd);
__attribute__((deprecated))
int hb_mc_get_recv_vacancy (uint8_t fd);

static uint8_t num_dev = 0;
static char *ocl_table[8] = {(char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0};

/*
 * Formats a Manycore request packet.
 * @param packet packet struct that this function will populate. caller must allocate.
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * assumes all fields are <= 32
 * */
__attribute__((deprecated))
static inline int hb_mc_format_request_packet(uint8_t fd, hb_mc_request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, hb_mc_packet_op_t opcode) {
        uint32_t intf_coord_x, intf_coord_y;

        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &intf_coord_x) != HB_MC_SUCCESS ||
           hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &intf_coord_y) != HB_MC_SUCCESS)
                return HB_MC_FAIL;

	hb_mc_request_packet_set_x_dst(packet, x);
	hb_mc_request_packet_set_y_dst(packet, y);
	hb_mc_request_packet_set_x_src(packet, (uint8_t)intf_coord_x);
	hb_mc_request_packet_set_y_src(packet, (uint8_t)intf_coord_y);
	hb_mc_request_packet_set_data(packet, data);
	hb_mc_request_packet_set_mask(packet, HB_MC_PACKET_REQUEST_MASK_WORD);
	hb_mc_request_packet_set_op(packet, opcode);
	hb_mc_request_packet_set_addr(packet, addr);
}

#ifdef __cplusplus
}
#endif
#endif
