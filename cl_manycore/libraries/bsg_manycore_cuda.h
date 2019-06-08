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


// Base EPA of all parameters for cuda lite runtime software
#define HB_MC_CUDA_TILE_PARAMETERS_BASE_EPA	0x1000
// EPA of pointer to kernel binary in tile. 
#define HB_MC_CUDA_TILE_KERNEL_PTR_EPA		(HB_MC_CUDA_TILE_PARAMETERS_BASE_EPA + 0x0000)
// EPA of argument count of kernel in tile.
#define HB_MC_CUDA_TILE_ARGC_PTR_EPA		(HB_MC_CUDA_TILE_PARAMETERS_BASE_EPA + 0x0004)  
// EPA of pointer to argument list of kernel in tile.
#define HB_MC_CUDA_TILE_ARGV_PTR_EPA		(HB_MC_CUDA_TILE_PARAMETERS_BASE_EPA + 0x0008)
// EPA of finish signal memory location in tile.
#define HB_MC_CUDA_TILE_FINISH_SIGNAL_PTR_EPA	(HB_MC_CUDA_TILE_PARAMETERS_BASE_EPA + 0x000C)  
// Kernel is not loaded into tile if kernel poitner equals this value.
#define HB_MC_CUDA_KERNEL_NOT_LOADED		0x0001	
// The begining of section in host memory intended for tile groups to write finish signals into
#define HB_MC_CUDA_HOST_FINISH_SIGNAL_BASE_ADDR	0xF000	


typedef uint8_t tile_group_id_t;
typedef uint8_t grid_id_t;
typedef int hb_mc_allocator_id_t;


typedef enum {
	HB_MC_TILE_GROUP_STATUS_INITIALIZED=0,
	HB_MC_TILE_GROUP_STATUS_ALLOCATED=1,
	HB_MC_TILE_GROUP_STATUS_LAUNCHED=2,
	HB_MC_TILE_GROUP_STATUS_FINISHED=3,
} hb_mc_tile_group_status_t ;


typedef struct {
	hb_mc_coordinate_t coord;
	hb_mc_coordinate_t origin;	
	hb_mc_coordinate_t tile_group_id;
	uint8_t free;
} hb_mc_tile_t;

typedef struct {
	char *name;
	uint32_t argc;
	uint32_t *argv;
	hb_mc_epa_t finish_signal_addr;
} hb_mc_kernel_t;

typedef struct {
	hb_mc_coordinate_t id;
	grid_id_t grid_id;
	hb_mc_dimension_t grid_dim;
	hb_mc_tile_group_status_t status;
	hb_mc_coordinate_t origin;
	hb_mc_dimension_t dim;
	hb_mc_eva_map_t *map;
	hb_mc_kernel_t *kernel;
} hb_mc_tile_group_t;


typedef struct {
	hb_mc_dimension_t dim;
	hb_mc_coordinate_t origin;
	hb_mc_tile_t* tiles;
} hb_mc_mesh_t;


typedef struct {
	hb_mc_allocator_id_t id;
	const char *name; 
	void *memory_manager;
} hb_mc_allocator_t;


typedef struct {
	char* bin_name;
	unsigned char* bin;
	size_t bin_size;
	hb_mc_allocator_t *allocator;
} hb_mc_program_t;


typedef struct {
	uint8_t fd;
	hb_mc_manycore_t *mc;
	hb_mc_program_t *program;
	hb_mc_mesh_t *mesh;
	hb_mc_tile_group_t *tile_groups;
	uint32_t num_tile_groups;
	uint32_t tile_group_capacity;
	uint8_t num_grids;
} hb_mc_device_t; 


enum hb_mc_memcpy_kind {hb_mc_memcpy_to_device = 0, hb_mc_memcpy_to_host = 1};



/**
 * Initializes the manycor struct, and a mesh structure inside
 * device struct with list of all tiles and their cooridnates 
 * @param[in]  device        Pointer to device
 * @param[in]  name          Device name
 * @param[in]  id            Device id
 * @param[in]  dim           Tile pool (mesh) dimensions
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_device_init (	hb_mc_device_t *device,
			char *name,
			hb_mc_manycore_id_t id,
			hb_mc_dimension_t dim_x);



/**
 * Initializes Manycore tiles so that they may run kernels.
 * @param[in]  device        Pointer to device
 * @parma[in]  bin_name      Name of binary elf file
 * @param[in]  id            Id of program's memory allocator
 * @param[in]  alloc_name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
__attribute__((warn_unused_result))
int hb_mc_device_program_init (	hb_mc_device_t *device,
				char *bin_name,
				const char *alloc_name,
				hb_mc_allocator_id_t id);



/**
 * Initializes a program's memory allcoator and creates a memroy manager
 * @param[in]  program       Pointer to program
 * @param[in]  id            Id of program's memory allocator
 * @param[in]  name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
__attribute__((warn_unused_result))
int hb_mc_program_allocator_init (	const hb_mc_config_t *cfg,
					hb_mc_program_t *allocator,
					const char *name,
					hb_mc_allocator_id_t id);



/**
 * Deletes memory manager, device and manycore struct, and freezes all tiles in device.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_device_finish (hb_mc_device_t *device);



/**
 * Allocates memory on device DRAM
 * @param[in]  device        Pointer to device
 * @parma[in]  size          Size of requested memory
 * @param[out] eva           Eva address of the allocated memory
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_device_malloc (hb_mc_device_t *device, uint32_t size, hb_mc_eva_t *eva);





/**
 * Frees memory on device DRAM
 * @param[in]  device        Pointer to device
 * @param[out] eva           Eva address of the memory to be freed
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result)) 
int hb_mc_device_free (hb_mc_device_t *device, hb_mc_eva_t eva);






/**
 * Iterates over all tile groups inside device,
 * allocates those that fit in mesh and launches them. 
 * API remains in this function until all tile groups
 * have successfully finished execution.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_device_tile_groups_execute (hb_mc_device_t *device);



/**
 * Checks to see if all tile groups in a device are finished.
 * @param[in]  device        Pointer to device
 * returns HB_MC_SUCCESS if all tile groups are finished, and HB_MC_FAIL otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_device_all_tile_groups_finished(hb_mc_device_t *device);



/**
 * Waits for a tile group to send a finish packet to device.
 * @param[in]  device        Pointer to device
 * return HB_MC_SUCCESS after a tile group is finished, gets stuck in infinite look if no tile group finishes.
 */
__attribute__((warn_unused_result))
int hb_mc_device_wait_for_tile_group_finish_any(hb_mc_device_t *device);




/**
 * Copies a buffer from src on the host/device DRAM to dst on device DRAM/host.
 * @param[in]  device        Pointer to device
 * @parma[in]  src           EVA address of src 
 * @param[in]  name          EVA address of dst
 * @param[in]  count         Size of buffer to be copied
 * @param[in]  hb_mc_memcpy_kind  Direction of copy (hb_mc_memcpy_to_device / hb_mc_memcpy_to_host)
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_device_memcpy (	hb_mc_device_t *device,
				void *dst,
				const void *src,
				uint32_t count,
				enum hb_mc_memcpy_kind kind);






/**
 * Takes in a hb_mc_device_t struct and initializes a mesh of tile in the Manycore device.
 * @param[in]  device        Pointer to device
 * @parma[in]  dim           X/Y dimensions of the tile pool (mesh) to be initialized
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_mesh_init (hb_mc_device_t *device, hb_mc_dimension_t dim); 




/**
 * Takes the grid size, tile group dimensions, kernel name, argc,
 * argv* and the finish signal address, calls hb_mc_tile_group_enqueue
 * to initialize all tile groups for grid.
 * @param[in]  device        Pointer to device
 * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
 * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
 * @param[in]  name          Kernel name to be executed on tile groups in grid
 * @param[in]  argc          Number of input arguments to kernel
 * @param[in]  argv          List of input arguments to kernel
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_grid_init (	hb_mc_device_t *device,
			hb_mc_dimension_t grid_dim,
			hb_mc_dimension_t tg_dim,
			char *name, uint32_t argc, uint32_t argv[]);




/**
 * Searches for a free tile group inside the device mesh and allocoates it,
 * and sets the dimensions, origin, and id of tile group.
 * @param[in]  device        Pointer to device
 * @param[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_tile_group_allocate_tiles(hb_mc_device_t *device, hb_mc_tile_group_t *tg);  




/**
 * Takes the kernel name, argc, argv* and the finish signal address,
 * and initializes a kernel and passes it to tilegroup.
 * @param[in]  device        Pointer to device
 * @parma[in]  grid_id       Id of grid to which the tile group belongs
 * @param[in]  tg_id         Id of tile group
 * @param[in]  grid_dim      X/Y dimensions of grid to which tile group belongs
 * @parma[in]  dim           X/Y dimensions of tie pool (mesh)
 * @param[in]  name          Kernel name that is to be executed on tile group
 * @param[in]  argc          Number of input arguments to the kernel
 * @param[in]  argv          List of input arguments to the kernel
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
__attribute__((warn_unused_result))
int hb_mc_tile_group_enqueue(	hb_mc_device_t *device,
				grid_id_t grid_id,
				hb_mc_coordinate_t tg_id,
				hb_mc_dimension_t grid_dim,
				hb_mc_dimension_t dim,
				char *name, uint32_t argc, uint32_t argv[]);




/**
 * Launches a tile group by sending packets to each tile in the
 * tile group setting the argc, argv, finish_addr and kernel pointer.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if tile group is launched successfully and HB_MC_FAIL otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_tile_group_launch(hb_mc_device_t *device, hb_mc_tile_group_t *tg);




/**
 * De-allocates all tiles in tile group, and resets their tile-group id and origin in the device book keeping.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if tile group is launched successfully and HB_MC_FAIL otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_tile_group_deallocate_tiles(hb_mc_device_t *device, hb_mc_tile_group_t *tg);




#ifdef __cplusplus
}
#endif

#endif
