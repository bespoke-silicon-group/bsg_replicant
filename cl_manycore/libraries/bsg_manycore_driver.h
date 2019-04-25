#ifndef BSG_MANYCORE_DRIVER_H
#define BSG_MANYCORE_DRIVER_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif


#include <stdint.h>
#include <limits.h>
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

int hb_mc_init_host (uint8_t *fd);
typedef uint32_t eva_id_t;
typedef struct {
	uint8_t x;
	uint8_t y;
	uint8_t origin_x;
	uint8_t origin_y;
} tile_t;

typedef uint32_t eva_t;

typedef struct {
	uint32_t x;
	uint32_t y; 
	uint32_t epa;
} npa_t;

typedef enum __hb_mc_direction_t {  /* MIN & MAX are not fifo directions, they are used for iteration over fifos. Update MAX if you add a fifo direction. */
	HB_MC_MMIO_FIFO_MIN = 0,
	HB_MC_MMIO_FIFO_TO_DEVICE = 0,
	HB_MC_MMIO_FIFO_TO_HOST = 1,
	HB_MC_MMIO_FIFO_MAX = 2
} hb_mc_direction_t;

int hb_mc_write_fifo (uint8_t fd, hb_mc_direction_t dir, hb_mc_packet_t *packet);
int hb_mc_get_fifo_occupancy (uint8_t fd, hb_mc_direction_t dir, uint32_t *occupancy_p);
int hb_mc_read_fifo (uint8_t fd, hb_mc_direction_t dir, hb_mc_packet_t *packet);
int hb_mc_drain_fifo (uint8_t fd, hb_mc_direction_t dir); 
int hb_mc_clear_int (uint8_t fd, hb_mc_direction_t dir);
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

int hb_mc_get_config (uint8_t fd, hb_mc_config_id_t id, uint32_t *cfg);
int hb_mc_get_recv_vacancy (uint8_t fd);
int hb_mc_can_read (uint8_t fd, uint32_t size);
int hb_mc_check_device (uint8_t fd);
uint8_t hb_mc_get_manycore_dimension_x ();
uint8_t hb_mc_get_manycore_dimension_y (); 
void hb_mc_format_request_packet(hb_mc_request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, hb_mc_packet_op_t opcode);
int hb_mc_eva_to_npa (eva_id_t eva_id, eva_t eva, npa_t *npa);
void hb_mc_device_sync (uint8_t fd, hb_mc_request_packet_t *finish);
int hb_mc_freeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_set_tile_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t x_cord, uint8_t y_cord);
void create_tile_group(tile_t tiles[], uint32_t num_tiles_x, uint32_t num_tiles_y, uint32_t origin_x, uint32_t origin_y);
int hb_mc_npa_to_eva (eva_id_t eva_id, npa_t *npa, eva_t *eva);
int hb_mc_init_cache_tag(uint8_t fd, uint8_t x, uint8_t y);

static uint8_t num_dev = 0;
static char *ocl_table[8] = {(char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0};


#ifdef __cplusplus
}
#endif
#endif
