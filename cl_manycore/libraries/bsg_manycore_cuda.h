#ifndef BSG_MANYCORE_CUDA_H
#define BSG_MANYCORE_CUDA_H

#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_mem.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_mem.h"
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t x;
	uint8_t y;
	uint8_t origin_x;
	uint8_t origin_y;
} tile_t;

int hb_mc_device_init (uint8_t fd, eva_id_t eva_id, char *elf, tile_t *tiles, uint32_t num_tiles);
int hb_mc_device_finish (uint8_t fd, eva_id_t eva_id, tile_t *tiles, uint32_t num_tiles);
int hb_mc_device_malloc (eva_id_t eva_id, uint32_t size, /*out*/ eva_t *eva);
int hb_mc_device_free (eva_id_t eva_id, eva_t eva);

void hb_mc_cuda_sync (uint8_t fd, tile_t *tile);
void hb_mc_device_sync (uint8_t fd, hb_mc_request_packet_t *finish);
int hb_mc_device_launch (uint8_t fd, eva_id_t eva_id, char *kernel, uint32_t argc, uint32_t argv[], char *elf, tile_t tiles[], uint32_t num_tiles);
void _hb_mc_get_mem_manager_info(eva_id_t eva_id, uint32_t *start, uint32_t *size); /* TODO: Remove; this is for testing only */

enum hb_mc_memcpy_kind {hb_mc_memcpy_to_device = 0, hb_mc_memcpy_to_host = 1};
int hb_mc_device_memcpy (uint8_t fd, eva_id_t eva_id, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind);

void create_tile_group(tile_t tiles[], uint32_t num_tiles_x, uint32_t num_tiles_y, uint32_t origin_x, uint32_t origin_y);
#ifdef __cplusplus
}
#endif

#endif
