#ifndef BSG_MANYCORE_CUDA_H
#define BSG_MANYCORE_CUDA_H
#include <bsg_manycore_features.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_mem.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t tile_group_id_t;

typedef enum {
	HB_MC_TILE_GROUP_STATUS_INITIALIZED=0,
	HB_MC_TILE_GROUP_STATUS_ALLOCATED=1,
	HB_MC_TILE_GROUP_STATUS_LAUNCHED=2,
	HB_MC_TILE_GROUP_STATUS_FINISHED=3,
} tile_group_status_t ;


typedef struct {
	uint8_t x;
	uint8_t y;
	uint8_t origin_x;
	uint8_t origin_y;
	uint8_t tile_group_id;
	uint8_t free;
} tile_t;

typedef struct {
	char *name;
	uint32_t argc;
	uint32_t *argv;
	uint32_t finish_signal_addr;
} kernel_t;

typedef struct {
	tile_group_id_t id;
	tile_group_status_t status;
	uint8_t origin_x;
	uint8_t origin_y;
	uint8_t dim_x;
	uint8_t dim_y;
	kernel_t *kernel;
} tile_group_t;


typedef struct {
	uint8_t dim_x;
	uint8_t dim_y;
	uint8_t origin_x;
	uint8_t origin_y;
	tile_t* tiles;
} grid_t;


typedef struct {
	uint8_t fd;
	eva_id_t eva_id; 
	grid_t *grid;
	char* elf;
	tile_group_t *tile_groups;
	uint32_t num_tile_groups;
} device_t; 



int hb_mc_device_init (device_t *device, eva_id_t eva_id, char *elf, uint8_t dim_x, uint8_t dim_y, uint8_t origin_x, uint8_t origin_y);
int hb_mc_device_finish (device_t *device);
int hb_mc_device_malloc (device_t *device, uint32_t size, /*out*/ eva_t *eva);
int hb_mc_device_free (device_t *device, eva_t eva);
int hb_mc_device_launch (device_t *device);
int hb_mc_device_add_tile_group (device_t *device, tile_group_t *tg);
int hb_mc_device_all_tile_groups_finished(device_t *device);
int hb_mc_device_wait_for_tile_group_finish(device_t *device);

enum hb_mc_memcpy_kind {hb_mc_memcpy_to_device = 0, hb_mc_memcpy_to_host = 1};
int hb_mc_device_memcpy (device_t *device, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind);

int hb_mc_grid_init (device_t *device, uint8_t dim_x, uint8_t dim_y, uint8_t origin_x, uint8_t origin_y); 


int hb_mc_tile_group_allocate(device_t *device, tile_group_t *tg);  
int hb_mc_tile_group_init(device_t *device, tile_group_t *tg,  uint8_t dim_x, uint8_t dim_y, char *name, uint32_t argc, uint32_t argv[], uint32_t finish_signal_addr);
int hb_mc_tile_group_launch(device_t *device, tile_group_t *tg);
int hb_mc_tile_group_sync(device_t *device, tile_group_t *tg); 
int hb_mc_tile_group_deallocate(device_t *device, tile_group_t *tg);
int hb_mc_wait_for_packet(device_t *device, hb_mc_request_packet_t *packet);

void hb_mc_cuda_sync (uint8_t fd, tile_t *tile);
void _hb_mc_get_mem_manager_info(eva_id_t eva_id, uint32_t *start, uint32_t *size); /* TODO: Remove; this is for testing only */


void create_tile_group(tile_t tiles[], uint32_t num_tiles_x, uint32_t num_tiles_y, uint32_t origin_x, uint32_t origin_y);
#ifdef __cplusplus
}
#endif

#endif
