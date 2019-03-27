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

int hb_mc_check_dim (uint8_t fd);      
int hb_mc_write_fifo (uint8_t fd, uint8_t n, hb_mc_packet_t *packet);
int hb_mc_get_fifo_occupancy (uint8_t fd, uint8_t n, uint32_t *occupancy_p);
int hb_mc_read_fifo (uint8_t fd, uint8_t n, hb_mc_packet_t *packet);
int hb_mc_clear_int (uint8_t fd, uint8_t n);
int hb_mc_get_host_credits (uint8_t fd);
int hb_mc_all_host_req_complete(uint8_t fd);
int hb_mc_get_recv_vacancy (uint8_t fd);
int hb_mc_can_read (uint8_t fd, uint32_t size);
int hb_mc_check_device (uint8_t fd);
uint8_t hb_mc_get_num_x ();
uint8_t hb_mc_get_num_y (); 
void hb_mc_format_request_packet(hb_mc_request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode);
int hb_mc_init_device (uint8_t fd, eva_id_t eva_id, char *elf, tile_t *tiles, uint32_t num_tiles);
int hb_mc_device_finish (uint8_t fd, eva_id_t eva_id, tile_t *tiles, uint32_t num_tiles);
int hb_mc_device_malloc (eva_id_t eva_id, uint32_t size, /*out*/ eva_t *eva);
int hb_mc_device_free (eva_id_t eva_id, eva_t eva);
int hb_mc_eva_to_npa (eva_id_t eva_id, eva_t eva, npa_t *npa);
void hb_mc_device_sync (uint8_t fd, hb_mc_request_packet_t *finish);
void hb_mc_cuda_sync (uint8_t fd, tile_t *tile);
int hb_mc_device_launch (uint8_t fd, eva_id_t eva_id, char *kernel, uint32_t argc, uint32_t argv[], char *elf, tile_t tiles[], uint32_t num_tiles);
void _hb_mc_get_mem_manager_info(eva_id_t eva_id, uint32_t *start, uint32_t *size); /* TODO: Remove; this is for testing only */
void create_tile_group(tile_t tiles[], uint32_t num_tiles_x, uint32_t num_tiles_y, uint32_t origin_x, uint32_t origin_y);

static uint8_t num_dev = 0;
static char *ocl_table[8] = {(char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0};

#define MAX_CREDITS 16

enum hb_mc_memcpy_kind {hb_mc_memcpy_to_device = 0, hb_mc_memcpy_to_host = 1};
int hb_mc_device_memcpy (uint8_t fd, eva_id_t eva_id, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind);

#ifdef __cplusplus
}
#endif
#endif
