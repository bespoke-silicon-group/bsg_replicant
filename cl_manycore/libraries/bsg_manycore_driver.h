#ifndef BSG_MANYCORE_DRIVER_H
#define BSG_MANYCORE_DRIVER_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif

#include <stdint.h>
#ifndef COSIM
	#include <bsg_manycore_packet.h>
#else
	#include "bsg_manycore_packet.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef COSIM
static char *hb_mc_mmap_ocl (uint8_t fd);
#endif

/* hb_mc_fifo_rx_t identifies the type of a packet receive operation*/
typedef enum __hb_mc_direction_t {  /*  */
	HB_MC_MMIO_FIFO_MIN = 0,
	HB_MC_MMIO_FIFO_TO_DEVICE = 0,
	HB_MC_MMIO_FIFO_TO_HOST = 1,
	HB_MC_MMIO_FIFO_MAX = 1
} hb_mc_direction_t;

/* 
 * hb_mc_fifo_rx_t identifies the type of a packet receive operation. MIN & MAX
 * are not fifo directions, they are used for iteration over fifos. 
 */
typedef enum __hb_mc_fifo_rx_t {  
	HB_MC_FIFO_RX_RSP = HB_MC_MMIO_FIFO_TO_DEVICE,
	HB_MC_FIFO_RX_REQ = HB_MC_MMIO_FIFO_TO_HOST
} hb_mc_fifo_rx_t;

/* 
 * hb_mc_fifo_tx_t identifies the type of a packet transmit operation. MIN & MAX
 * are not fifo directions, they are used for iteration over fifos. 
 */
typedef enum __hb_mc_fifo_tx_t {  /*  */
	HB_MC_FIFO_TX_REQ = HB_MC_MMIO_FIFO_TO_DEVICE,
	HB_MC_FIFO_TX_RSP = HB_MC_MMIO_FIFO_TO_HOST
} hb_mc_fifo_tx_t;

inline hb_mc_direction_t hb_mc_get_rx_direction(hb_mc_fifo_rx_t d){
	return (hb_mc_direction_t)d;
}

inline hb_mc_direction_t hb_mc_get_tx_direction(hb_mc_fifo_tx_t d){
	return (hb_mc_direction_t)d;
}

int hb_mc_fifo_transmit (uint8_t fd, hb_mc_fifo_tx_t type, hb_mc_packet_t *packet);
int hb_mc_fifo_receive (uint8_t fd, hb_mc_fifo_rx_t type, hb_mc_packet_t *packet);

void hb_mc_format_request_packet(hb_mc_request_packet_t *packet, 
				uint32_t addr, uint32_t data, 
				uint8_t x, uint8_t y, 
				hb_mc_packet_op_t opcode);

int hb_mc_get_host_credits (uint8_t fd);
int hb_mc_all_host_req_complete(uint8_t fd);

typedef enum __hb_mc_config_id_t {
	HB_MC_CONFIG_VERSION = 0,
	HB_MC_CONFIG_COMPLIATION_DATE = 1,
	HB_MC_CONFIG_NETWORK_ADDR_WIDTH = 2,
	HB_MC_CONFIG_NETWORK_DATA_WIDTH = 3,
	HB_MC_CONFIG_DEVICE_DIM_X = 4,
	HB_MC_CONFIG_DEVICE_DIM_Y = 5,
	HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X = 6,
	HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y = 7,
	HB_MC_CONFIG_NOT_IMPLEMENTED = 8,
	HB_MC_CONFIG_REPO_STL_HASH = 9,
	HB_MC_CONFIG_REPO_MANYCORE_HASH = 10,
	HB_MC_CONFIG_REPO_F1_HASH = 11,
	HB_MC_CONFIG_MAX = 12
} hb_mc_config_id_t;
int hb_mc_get_config(uint8_t fd, hb_mc_config_id_t id, uint32_t *cfg);
uint8_t hb_mc_get_manycore_dimension_x();
uint8_t hb_mc_get_manycore_dimension_y(); 

int hb_mc_host_finish(uint8_t fd);
int hb_mc_init_host(uint8_t *fd);
int hb_mc_check_device(uint8_t fd);

int hb_mc_get_recv_vacancy (uint8_t fd);

static uint8_t num_dev = 0;
static char *ocl_table[8] = {(char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0};


#ifdef __cplusplus
}
#endif
#endif
