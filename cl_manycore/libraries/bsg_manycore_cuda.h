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
typedef uint8_t grid_id_t;

typedef enum {
	HB_MC_TILE_GROUP_STATUS_INITIALIZED=0,
	HB_MC_TILE_GROUP_STATUS_ALLOCATED=1,
	HB_MC_TILE_GROUP_STATUS_LAUNCHED=2,
	HB_MC_TILE_GROUP_STATUS_FINISHED=3,
} tile_group_status_t ;


typedef struct {
	hb_mc_coordinate_t coord;
	hb_mc_coordinate_t origin;	
	hb_mc_coordinate_t tile_group_id;
	uint8_t free;
} tile_t;

typedef struct {
	char *name;
	uint32_t argc;
	uint32_t *argv;
	uint32_t finish_signal_addr;
} kernel_t;

typedef struct {
	hb_mc_coordinate_t id;
	grid_id_t grid_id;
	hb_mc_dimension_t grid_dim;
	tile_group_status_t status;
	hb_mc_coordinate_t origin;
	hb_mc_dimension_t dim;
	hb_mc_eva_map_t *map;
	kernel_t *kernel;
} tile_group_t;


typedef struct {
	hb_mc_dimension_t dim;
	hb_mc_coordinate_t origin;
	tile_t* tiles;
} mesh_t;


typedef struct {
	hb_mc_manycore_t *mc;
	uint8_t fd;
	eva_id_t eva_id; 
	mesh_t *mesh;
	char* bin_name;
	unsigned char* bin;
	size_t bin_size;
	tile_group_t *tile_groups;
	uint32_t num_tile_groups;
	uint32_t tile_group_capacity;
	uint8_t num_grids;
} device_t; 



int hb_mc_device_init (device_t *device, eva_id_t eva_id, char *bin_name, char *name, hb_mc_manycore_id_t id, hb_mc_dimension_t dim_x);
int hb_mc_device_finish (device_t *device);
int hb_mc_device_malloc (device_t *device, uint32_t size, /*out*/ eva_t *eva);
int hb_mc_device_free (device_t *device, eva_t eva);
int hb_mc_device_tile_groups_execute (device_t *device);
int hb_mc_device_all_tile_groups_finished(device_t *device);
int hb_mc_device_wait_for_tile_group_finish_any(device_t *device);

enum hb_mc_memcpy_kind {hb_mc_memcpy_to_device = 0, hb_mc_memcpy_to_host = 1};
int hb_mc_device_memcpy (device_t *device, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind);

int hb_mc_mesh_init (device_t *device, hb_mc_dimension_t dim); 

int hb_mc_grid_init (device_t *device, hb_mc_dimension_t grid_dim, hb_mc_dimension_t tg_dim, char *name, uint32_t argc, uint32_t argv[]);

int hb_mc_tile_group_allocate_tiles(device_t *device, tile_group_t *tg);  
int hb_mc_tile_group_enqueue(device_t *device, grid_id_t grid_id, hb_mc_coordinate_t tg_id, hb_mc_dimension_t grid_dim, hb_mc_dimension_t dim, char *name, uint32_t argc, uint32_t argv[]);
int hb_mc_tile_group_launch(device_t *device, tile_group_t *tg);
int hb_mc_tile_group_deallocate_tiles(device_t *device, tile_group_t *tg);

void _hb_mc_get_mem_manager_info(eva_id_t eva_id, uint32_t *start, uint32_t *size); /* TODO: Remove; this is for testing only */


void create_tile_group(tile_t tiles[], hb_mc_dimension_t num_tiles, hb_mc_coordinate_t origin);
#ifdef __cplusplus
}
#endif

#endif
