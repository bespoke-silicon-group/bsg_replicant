// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_cuda_barrier.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_memory_manager.h>
#include <bsg_manycore_elf.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_origin_eva_map.h>
#include <bsg_manycore_config_pod.h>

#ifdef __cplusplus
#include <cstring>
#else
#include <string.h>
#endif



////////////////////
// Malloc helpers //
////////////////////
#define XMALLOC_N(ptr, n)                                               \
        do {                                                            \
                ptr = reinterpret_cast<decltype(ptr)>                   \
                        (malloc(sizeof(*(ptr)) * (n)));                 \
                if (ptr == NULL) {                                      \
                        bsg_pr_err("%s: failed to allocate '%s': %s\n", \
                                   __func__,                            \
                                   #ptr,                                \
                                   hb_mc_strerror(HB_MC_NOMEM));        \
                        return HB_MC_NOMEM;                             \
                }                                                       \
        } while (0)

#define XMALLOC(ptr)                            \
        XMALLOC_N(ptr, 1)

#define XREALLOC(ptr, n)                                                \
        do {                                                            \
                decltype(ptr) __tmp;                                    \
                __tmp = reinterpret_cast<decltype(__tmp)>               \
                                         (realloc(ptr, sizeof(*ptr) * (n))); \
                if (__tmp == NULL) {                                    \
                        bsg_pr_err("%s: failed to reallocate '%s': %s\n", \
                                   __func__,                            \
                                   #ptr,                                \
                                   hb_mc_strerror(HB_MC_NOMEM));        \
                        return HB_MC_NOMEM;                             \
                }                                                       \
                ptr = __tmp;                                            \
        } while (0)

#define XSTRDUP(dup_str, og_str)                                        \
        do {                                                            \
                dup_str = strdup(og_str);                               \
                if (dup_str == NULL) {                                  \
                        bsg_pr_err("%s: failed to duplicate '%s'('%s'): %s\n", \
                                   __func__,                            \
                                   #og_str,                             \
                                   og_str,                              \
                                   hb_mc_strerror(HB_MC_NOMEM));        \
                        return HB_MC_NOMEM;                             \
                }                                                       \
        } while(0)                                                      \

///////////////
// Constants //
///////////////
static const hb_mc_dimension_t HB_MC_MESH_FULL_CORE = HB_MC_DIMENSION(0,0);

////////////////////
// Kernel helpers //
////////////////////
/**
 * Initialize a kernel
 */
__attribute__((warn_unused_result))
static int kernel_init(hb_mc_kernel_t *kernel, const char *name, uint32_t argc, const uint32_t *argv)
{
        XSTRDUP(kernel->name, name);
        XMALLOC_N(kernel->argv, argc);
        memcpy(const_cast<uint32_t*>(kernel->argv), argv, argc * sizeof(*argv));
        kernel->argc = argc;
        kernel->refcount = 0;

        return HB_MC_SUCCESS;
}

/**
 * Cleanup a kernel
 */
__attribute__((warn_unused_result))
static int kernel_exit(hb_mc_kernel_t *kernel)
{
        if (kernel->refcount > 0) {
                bsg_pr_err("%s: failed to cleanup kernel '%s': reference count not zero\n",
                           __func__,
                           kernel->name);
                return HB_MC_FAIL;
        }

        free(const_cast<char*>(kernel->name));
        kernel->name = NULL;

        free(const_cast<uint32_t*>(kernel->argv));
        kernel->argv = NULL;

        kernel->refcount = 0;
        kernel->argc = 0;

        return HB_MC_SUCCESS;
}

////////////////////////
// Tile group helpers //
////////////////////////
__attribute__((warn_unused_result))
static int hb_mc_device_pod_tile_group_init(hb_mc_device_t* device,
                                            hb_mc_pod_t *pod,
                                            hb_mc_tile_group_t *tg,
                                            grid_id_t grid_id,
                                            hb_mc_coordinate_t tg_id,
                                            hb_mc_dimension_t dim,
                                            hb_mc_kernel_t *kernel);

__attribute__((warn_unused_result))
static int hb_mc_device_pod_tile_group_exit(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_group_t *tg);

/////////////////////
// Program helpers //
/////////////////////
/**
 * Initializes program's memory allocator and creates a memory manager
 * @param[in]  program       Pointer to program
 * @param[in]  id            Id of program's meomry allocator
 * @param[in]  name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_program_allocator_init (const hb_mc_config_t *cfg,
                                         hb_mc_program_t *program,
                                         const char *name,
                                         hb_mc_allocator_id_t id) {
        int error;

        program->allocator = (hb_mc_allocator_t *) malloc (sizeof (hb_mc_allocator_t));
        if (program->allocator == NULL) {
                bsg_pr_err("%s: failed to allcoat space on host for program's hb_mc_allocator_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }

        program->allocator->name = strdup(name);
        if (!program->allocator->name) {
                bsg_pr_err("%s: failed to copy allocator name to program->allocator struct.\n", __func__);
                return HB_MC_NOMEM;
        }
        program->allocator->id = id;

        hb_mc_eva_t program_end_eva;
        error = hb_mc_loader_symbol_to_eva(program->bin, program->bin_size, "_bsg_dram_end_addr", &program_end_eva);
        if (error != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to acquire _bsg_dram_end_addr eva from binary file.\n", __func__);
                return HB_MC_INVALID;
        }

        uint32_t alignment = hb_mc_config_get_vcache_block_size(cfg);
        uint32_t start = program_end_eva + alignment - (program_end_eva % alignment); /* start at the next aligned block */
        size_t dram_size = hb_mc_config_get_dram_size(cfg);
        program->allocator->memory_manager = (awsbwhal::MemoryManager *) new awsbwhal::MemoryManager(dram_size, start, alignment);

        return HB_MC_SUCCESS;
}






/**
 * Frees memroy and removes allocator object
 * @param[in]  allcator   Pointer to allocator struct
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned.
 */
static int hb_mc_program_allocator_exit (hb_mc_allocator_t *allocator) {
        if (!allocator) {
                bsg_pr_err("%s: calling exit on null allocator.\n", __func__);
                return HB_MC_INVALID;
        }


        // Free name
        const char* name;
        name = allocator->name;
        if (!name) {
                bsg_pr_err("%s: calling exit on allocator with null name.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((void *) name);
                allocator->name = NULL;
        }


        // Free memory manager
        const awsbwhal::MemoryManager *memory_manager;
        memory_manager = (awsbwhal::MemoryManager *) allocator->memory_manager;
        if (!memory_manager) {
                bsg_pr_err("%s: calling exit on allocator with null memory manager.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((awsbwhal::MemoryManager *) memory_manager);
                allocator->memory_manager = NULL;
        }
        free(allocator);

        return HB_MC_SUCCESS;
}

//////////////////
// Tile helpers //
//////////////////


/**
 * Calculates and returns a tile group's finish signal address based on tile group id and grid dimension
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return     finish_signal_addr
 */
static hb_mc_epa_t hb_mc_tile_group_get_finish_signal_addr(hb_mc_tile_group_t *tg) {
        hb_mc_epa_t finish_addr = HB_MC_CUDA_HOST_FINISH_SIGNAL_BASE_ADDR
                + ((hb_mc_coordinate_get_y(tg->id) * hb_mc_dimension_get_x(tg->grid_dim)
                    + hb_mc_coordinate_get_x(tg->id)) << 2);
        return finish_addr;
}




/**
 * Calculates and returns the flat index of a tile inside the mesh,
 * based on the mesh origin, mesh dimensions and the tile coordinates.
 * @param[in]  origin        Mesh origin coordinates
 * @param[in]  dim           Mesh dimensions
 * @parma[in]  coord         Absolute coordinates
 * @return     tile_id
 */
static hb_mc_idx_t hb_mc_get_tile_id (hb_mc_coordinate_t origin, hb_mc_dimension_t dim, hb_mc_coordinate_t coord) {
        hb_mc_coordinate_t relative_coord = hb_mc_coordinate_get_relative (origin, coord);
        hb_mc_idx_t tile_id = hb_mc_coordinate_to_index (relative_coord, dim);
        return tile_id;
}

/**
 * Set a global symbol value
 */
static int tile_set_symbol_val(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_t *tile,
                               const hb_mc_eva_map_t *map,
                               const char *symbol, uint32_t val)
{
        hb_mc_program_t *program = pod->program;
        hb_mc_eva_t symbol_dev;
        int r;

        r = hb_mc_loader_symbol_to_eva(program->bin, program->bin_size, symbol, &symbol_dev);
        if (r != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to find symbol '%s' in program '%s': %s\n",
                           __func__,
                           symbol,
                           program->bin_name,
                           hb_mc_strerror(r));
                return r;
        }

        bsg_pr_dbg("%s: device<%s>: program:<%s>: Setting symbol '%s' @ 0x%08" PRIx32 " = %08" PRIx32 "\n",
                   __func__, device->name, pod->program->bin_name, symbol, symbol_dev, val);

        BSG_MANYCORE_CALL(device->mc,
                          hb_mc_manycore_eva_write(device->mc, map, &tile->coord, &symbol_dev,
                                                   &val, sizeof(val)));

        return HB_MC_SUCCESS;
}

/**
 * Freeze a tile
 */
static int tile_freeze(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_t *tile)
{
#ifdef DEBUG
        char buf[256];
#endif
        bsg_pr_dbg("%s: device<%s>: freezing tile %s\n",
                   __func__, device->name, hb_mc_coordinate_to_string(tile->coord, buf, sizeof(buf)));
        BSG_MANYCORE_CALL(device->mc, hb_mc_tile_freeze(device->mc, &tile->coord));
        return HB_MC_SUCCESS;
}

/**
 * Unfreeze a tile
 */
static int tile_unfreeze(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_t *tile)
{
#ifdef DEBUG
        char buf[256];
#endif
        bsg_pr_dbg("%s: device<%s>: unfreezing tile %s\n",
                   __func__, device->name, hb_mc_coordinate_to_string(tile->coord, buf, sizeof(buf)));
        // before unfreezing, clear kernel ptr
        uint32_t kernel_not_loaded = HB_MC_CUDA_KERNEL_NOT_LOADED_VAL;
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, &default_map, "cuda_kernel_ptr", kernel_not_loaded));
        BSG_MANYCORE_CALL(device->mc, hb_mc_tile_unfreeze(device->mc, &tile->coord));
        return HB_MC_SUCCESS;
}

/**
 * Sets the CUDA runtime symbols for a  tile
 */
__attribute__((warn_unused_result))
static int tile_set_runtime_symbols(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_t *tile, hb_mc_tile_group_t *tg,
                                    uint32_t    argc, hb_mc_eva_t kernel_addr)
{
        const hb_mc_eva_map_t *map = tg->map;
        hb_mc_eva_t argv_addr = tg->argv_eva;
        hb_mc_npa_t finish_signal_npa = tg->finish_signal_npa;

        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "cuda_argc", argc));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "cuda_argv_ptr", argv_addr));

        hb_mc_eva_t finish_signal_addr;
        size_t sz;
        BSG_CUDA_CALL(hb_mc_npa_to_eva(device->mc, map, &tile->coord, &finish_signal_npa, &finish_signal_addr, &sz));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "cuda_finish_signal_addr", finish_signal_addr));

        // set the barrier pointer, if found
        if (tg->barcfg_eva != 0)
                BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__cuda_barrier_cfg", tg->barcfg_eva));


        // tiles wake-on-broken reservation on this address
        // this write wakes up the kernel and 'launches' it
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "cuda_kernel_ptr", kernel_addr));

        return HB_MC_SUCCESS;
}

/**
 * Sets the CUDA configuration symbols for a tile
 */
__attribute__((warn_unused_result))
static int tile_set_config_symbols(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_t *tile,
                                   hb_mc_eva_map_t *map,
                                   hb_mc_coordinate_t origin,
                                   hb_mc_coordinate_t tg_id,
                                   hb_mc_dimension_t tg_dim,
                                   hb_mc_dimension_t grid_dim)
{
        hb_mc_coordinate_t coord = hb_mc_coordinate_get_relative (origin, tile->coord);
        BSG_MANYCORE_CALL(device->mc, hb_mc_tile_set_origin_registers(device->mc, &tile->coord, &origin));

        // Set tile's tile group origin __bsg_grp_org_x/y symbols.
        hb_mc_idx_t origin_x = hb_mc_coordinate_get_x (origin);
        hb_mc_idx_t origin_y = hb_mc_coordinate_get_y (origin);
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_grp_org_x", origin_x));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_grp_org_y", origin_y));

        // Set tile's index __bsg_x/y symbols.
        // A tile's __bsg_x/y symbols represent its X/Y
        // coordinates with respect to the origin tile
        hb_mc_idx_t coord_x = hb_mc_coordinate_get_x (coord);
        hb_mc_idx_t coord_y = hb_mc_coordinate_get_y (coord);
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_x", coord_x));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_y", coord_y));

        // Set tile's __bsg_id symbol.
        // bsg_id uniquely identifies each tile in a tile group
        // Tile group is a 2D array of tiles that concurrently run a kernel inside the binary
        // __bsg_x/y represent the X/Y coordiantes of tile relative to tile group origin
        // The flat tile id is calculated using tile group dimensions
        // and the tile group X/Y coordiantes relative to tile group origin as follows:
        // __bsg_id = __bsg_y * __bsg_tile_group_dim_x + __bsg_x
        hb_mc_idx_t id = hb_mc_coordinate_get_y(coord) * hb_mc_dimension_get_x(tg_dim) + hb_mc_coordinate_get_x(coord);
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_id", id));

        // Set tile's tile group index __bsg_tile_group_id_x/y symbols.
        // Grid is a 2D array of tile groups representing an application
        // bsg_tile_group_id uniquely identifies each tile group in a grid.
        // The flat tile group id is calculated using the grid dimensions
        // and the tile group X/Y coordinates relative to grid origin as follows:
        // __bsg_tile_group_id = __bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x
        // bsg_tile_group_id is used in bsg_print_stat to distinguish between
        // unique tile group invocations that have been executed in sequence on the same tile(s).
        hb_mc_idx_t tg_id_x  = hb_mc_coordinate_get_x (tg_id);
        hb_mc_idx_t tg_id_y  = hb_mc_coordinate_get_y (tg_id);
        hb_mc_idx_t tg_id_id = tg_id_y * hb_mc_dimension_get_x(grid_dim) + tg_id_x;
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_tile_group_id_x", tg_id_x));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_tile_group_id_y", tg_id_y));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_tile_group_id",   tg_id_id));

        // Set tile's grid dimension __bsg_grid_dim_x/y symbol.
        hb_mc_idx_t grid_dim_x = hb_mc_dimension_get_x (grid_dim);
        hb_mc_idx_t grid_dim_y = hb_mc_dimension_get_y (grid_dim);
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_grid_dim_x", grid_dim_x));
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "__bsg_grid_dim_y", grid_dim_y));

        // Set tile's finish signal value  cuda_finish_signal_val symbol.
        uint32_t finish_signal_val = HB_MC_CUDA_FINISH_SIGNAL_VAL;
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "cuda_finish_signal_val", finish_signal_val));

        // Set tile's kernel not loaded value  cuda_kernel_not_loaded_val symbol.
        uint32_t kernel_not_loaded_val = HB_MC_CUDA_KERNEL_NOT_LOADED_VAL;
        BSG_CUDA_CALL(tile_set_symbol_val(device, pod, tile, map, "cuda_kernel_not_loaded_val", kernel_not_loaded_val));

        return HB_MC_SUCCESS;
}

//////////////////
// Mesh helpers //
//////////////////
static int mesh_num_tiles(hb_mc_mesh_t *mesh)
{
        return hb_mc_dimension_to_length(mesh->dim);
}

#define mesh_foreach_tile_id(mesh, id_var)                              \
        for(id_var = 0; id_var < mesh_num_tiles(mesh); id_var++)

#define mesh_foreach_tile(mesh, tile_ptr)                               \
        for (tile_ptr = mesh->tiles; tile_ptr != mesh->tiles + mesh_num_tiles(mesh); tile_ptr++)

/////////////////
// Pod helpers //
/////////////////
#define pod_foreach_tile_group(pod, tile_group_ptr)     \
        for (tile_group_ptr = pod->tile_groups; tile_group_ptr != pod->tile_groups+pod->num_tile_groups; tile_group_ptr++)

///////////////////////
// Iteration helpers //
///////////////////////
#define device_foreach_pod(device, pod_ptr)                             \
        for (pod_ptr = device->pods; pod_ptr != device->pods+device->num_pods; pod_ptr++)

static hb_mc_pod_id_t hb_mc_device_pod_to_pod_id(hb_mc_device_t *device, hb_mc_pod_t *pod)
{
        return pod - device->pods;
}

////////////////////
// Input Checkers //
////////////////////
#define CHECK_PTR(ptr)                                                  \
        do {                                                            \
                if (ptr == NULL) {                                      \
                        bsg_pr_err("%s: called with null input: '%s'\n", \
                                   __func__, #ptr);                     \
                        return HB_MC_INVALID;                           \
                }                                                       \
        } while (0)

#define CHECK_POD_ID(device, id)                                        \
        do {                                                            \
                if (id < 0 || id >= (device)->num_pods) {               \
                        bsg_pr_err("%s: Bad pod = %d: %d pods present\n", \
                                   __func__, id, (device)->num_pods);   \
                        return HB_MC_INVALID;                           \
                }                                                       \
        } while (0)


///////////////////////////////////////
// Device Initialization and Cleanup //
///////////////////////////////////////

/**
 * Initializes a pod for a device.
 */
static int hb_mc_device_pod_init(hb_mc_device_t *device,
                                 hb_mc_pod_t    *pod)
{
        pod->program             = NULL;
        pod->mesh                = NULL;
        pod->tile_groups         = NULL;
        pod->num_tile_groups     = 0;
        pod->tile_group_capacity = 0;
        pod->num_grids           = 0;
        pod->program_loaded      = 0;
        return HB_MC_SUCCESS;
}

/**
 * Cleans-up a pod for a device.
 */
static int hb_mc_device_pod_exit(hb_mc_device_t *device,
                                 hb_mc_pod_t    *pod)
{
        return HB_MC_SUCCESS;
}

/**
 * Initializes the manycore struct, and a mesh structure with default (maximum)
 * dimensions inside device struct with list of tiles and their coordinates
 * @param[in]  device        Pointer to device
 * @param[in]  name          Device name
 * @param[in]  id            Device id
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_init (hb_mc_device_t *device,
                       const char *name,
                       hb_mc_manycore_id_t id)
{
        // initialize manycore
        XMALLOC(device->mc);
        *(device->mc) = {0};

        BSG_MANYCORE_CALL(device->mc, hb_mc_manycore_init(device->mc, name, id))

        // enumerate pods
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device->mc);
        hb_mc_dimension_t pod_geometry = hb_mc_config_pods(cfg);
        int num_pods = hb_mc_dimension_to_length(pod_geometry);
        hb_mc_pod_t *pods;
        XMALLOC_N(pods, num_pods);
        device->pods = pods;
        device->num_pods = num_pods;
        device->default_pod_id = 0;
        device->default_mesh_dim = HB_MC_MESH_FULL_CORE;

        // initialize pods
        hb_mc_coordinate_t pod_coord;
        hb_mc_config_foreach_pod(pod_coord, cfg)
        {
                hb_mc_pod_id_t pid = hb_mc_coordinate_to_index(pod_coord, pod_geometry);
                hb_mc_pod_t *pod = &device->pods[pid];
                hb_mc_device_pod_init(device, pod);
                // set the pod coordinate
                pod->pod_coord = pod_coord;
        }

        // set name
        XSTRDUP(device->name, name);

        return HB_MC_SUCCESS;
}


/**
 * Initializes the manycor struct, and a mesh structure with custom
 * diemsnions inside device struct with list of all tiles and their coordinates
 * @param[in]  device        Pointer to device
 * @param[in]  name          Device name
 * @param[in]  id            Device id
 * @param[in]  dim           Tile pool (mesh) dimensions
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_init_custom_dimensions (hb_mc_device_t *device,
                                         const char *name,
                                         hb_mc_manycore_id_t id,
                                         hb_mc_dimension_t dim)
{
        BSG_CUDA_CALL(hb_mc_device_init(device, name, id));
        device->default_mesh_dim = dim;
        return HB_MC_SUCCESS;
}


/**
 * Deletes memory manager, device and manycore struct, and freezes all tiles in device.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_program_finish (hb_mc_device_t *device)
{

        // cleanup pod
        hb_mc_pod_id_t pod_id = device->default_pod_id;
        hb_mc_pod_t *pod = &device->pods[pod_id];
        BSG_CUDA_CALL(hb_mc_device_pod_program_finish(device, pod_id));

        // fence on all requests
        BSG_CUDA_CALL(hb_mc_manycore_host_request_fence(device->mc, -1));

        return HB_MC_SUCCESS;
}


/**
 * Deletes memory manager, device and manycore struct, and freezes all tiles in device.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_finish (hb_mc_device_t *device)
{

        // cleanup pods
        hb_mc_pod_id_t pod_id;
        hb_mc_device_foreach_pod_id(device, pod_id)
        {
                hb_mc_pod_t *pod = &device->pods[pod_id];
                BSG_CUDA_CALL(hb_mc_device_pod_program_finish(device, pod_id));
                BSG_CUDA_CALL(hb_mc_device_pod_exit(device, pod));
        }

        // fence on all requests
        BSG_CUDA_CALL(hb_mc_manycore_host_request_fence(device->mc, -1));

        // cleanup manycore
        BSG_CUDA_CALL(hb_mc_manycore_exit (device->mc));

        // free resources
        free(device->mc);
        free(device->pods);
        free(const_cast<char*>(device->name));

        return HB_MC_SUCCESS;
}



/*****************/
/* Pod Interface */
/*****************/
void hb_mc_program_options_default(hb_mc_program_options_t *popts)
{
        static char default_alloc_name [] = "anonymous-allocator";
        static char default_program_name [] = "anonymous-program";
        popts->alloc_name = default_alloc_name;
        popts->program_name = default_program_name;
        popts->alloc_id   = 0;
        popts->mesh_dim = HB_MC_DIMENSION(0,0);
        popts->move_bin_data = 0;
}

/********************************/
/* Pod Interface Initialization */
/********************************/

/**
 * Initializes a mesh.
 */
__attribute__((warn_unused_result))
static
int hb_mc_device_pod_mesh_init(hb_mc_device_t *device,
                               hb_mc_pod_t *pod,
                               const hb_mc_program_options_t *popts)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device->mc);
        hb_mc_dimension_t device_dim = hb_mc_config_get_dimension_vcore(cfg);
        hb_mc_dimension_t dim = popts->mesh_dim;

        // if dim is (0,0), set to size of vcore
        if (hb_mc_dimension_eq(HB_MC_MESH_FULL_CORE, dim)) {
                dim = hb_mc_config_get_dimension_vcore(cfg);
        }

        // if this device has a custom mesh dimension, use that
        if (!hb_mc_dimension_eq(HB_MC_MESH_FULL_CORE, device->default_mesh_dim)) {
                dim = device->default_mesh_dim;
        }

        // sanity check input dimensions
        if (hb_mc_dimension_get_x(dim) <= 0){
                bsg_pr_err("%s: Mesh X dimension (%d) not valid.\n", __func__, hb_mc_dimension_get_x(dim));
                return HB_MC_INVALID;
        }
        if (hb_mc_dimension_get_y(dim) <= 0){
                bsg_pr_err("%s: Mesh Y dimension (%d) not valid.\n", __func__, hb_mc_dimension_get_y(dim));
                return HB_MC_INVALID;
        }
        if (hb_mc_dimension_get_x(dim) > hb_mc_dimension_get_x(device_dim)){
                bsg_pr_err("%s: Mesh X dimension (%d) larger than device X dimension %d.\n",
                           __func__,
                           hb_mc_dimension_get_x(dim),
                           hb_mc_dimension_get_x(device_dim));
                return HB_MC_INVALID;
        }
        if (hb_mc_dimension_get_y(dim) > hb_mc_dimension_get_y(device_dim)){
                bsg_pr_err("%s: Mesh Y dimension (%d) larger than device Y dimension (%d).\n",
                           __func__,
                           hb_mc_dimension_get_y(dim),
                           hb_mc_dimension_get_y(device_dim));
                return HB_MC_INVALID;
        }

        // initialize mesh
        hb_mc_mesh_t *mesh;
        XMALLOC(mesh);
        if (mesh == NULL) {
                bsg_pr_err("%s: failed to allocate space on host for hb_mc_mesh_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }

        mesh->dim = dim;
        mesh->origin = hb_mc_config_pod_vcore_origin(cfg, pod->pod_coord);
        mesh->tiles = (hb_mc_tile_t *) malloc ( hb_mc_dimension_to_length(dim) * sizeof (hb_mc_tile_t));
        if (mesh->tiles == NULL) {
                bsg_pr_err("%s: failed to allocate space on host for hb_mc_tile_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }

        // initialize each tile and mark its status as 'free'
        hb_mc_idx_t x, y;
        foreach_x_y(x, y, mesh->origin, mesh->dim)
        {
                hb_mc_idx_t tile_id = hb_mc_get_tile_id(mesh->origin, mesh->dim, hb_mc_coordinate(x,y));
                mesh->tiles[tile_id].coord = hb_mc_coordinate(x, y);
                mesh->tiles[tile_id].origin = mesh->origin;
                mesh->tiles[tile_id].tile_group_id = hb_mc_coordinate(-1, -1);
                mesh->tiles[tile_id].status = HB_MC_TILE_STATUS_FREE;

        }

        pod->mesh = mesh;

        return HB_MC_SUCCESS;
}

/**
 * Initializes tile groups.
 */
__attribute__((warn_unused_result))
static
int hb_mc_device_pod_tile_groups_init(hb_mc_device_t *device,
                                      hb_mc_pod_t *pod)
{
        hb_mc_tile_group_t *groups;
        int capacity = 1;

        // allocate tile groups
        XMALLOC_N(groups, capacity);
        if (groups == NULL) {
                bsg_pr_err("%s: failed to allocated space for list of tile groups.\n", __func__);
                return HB_MC_NOMEM;
        }
        // initialize
        memset(groups, 0, sizeof(*groups) * capacity);
        pod->tile_groups = groups;
        pod->tile_group_capacity = capacity;
        pod->num_tile_groups = 0;

        return HB_MC_SUCCESS;

}
/**
 * Load a program
 */
__attribute__((warn_unused_result))
static
int hb_mc_device_pod_program_load (hb_mc_device_t *device, hb_mc_pod_t *pod)
{
        int r = HB_MC_SUCCESS;

        // Create list of tile coordinates
        hb_mc_coordinate_t tile_list[mesh_num_tiles(pod->mesh)];
        int tile_id;
        mesh_foreach_tile_id(pod->mesh, tile_id)
        {
                char tile_str[256];
                bsg_pr_dbg("%s: device<%s>: Adding tile %s to list\n",
                           __func__, device->name,
                           hb_mc_coordinate_to_string(pod->mesh->tiles[tile_id].coord, tile_str, sizeof(tile_str)));
                tile_list[tile_id] = pod->mesh->tiles[tile_id].coord;
        }

        // Freeze all tiles
        hb_mc_tile_t *tile;
        mesh_foreach_tile(pod->mesh, tile)
        {
                BSG_CUDA_CALL(tile_freeze(device, pod, tile));
        }


        // Load binary into all tiles
        r = hb_mc_loader_load (pod->program->bin,
                               pod->program->bin_size,
                               device->mc,
                               &default_map,
                               tile_list,
                               mesh_num_tiles(pod->mesh));
        if (r != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to load program '%s': %s\n",
                           __func__,
                           pod->program->bin_name,
                           hb_mc_strerror(r));
                return r;
        }

        // Set all tiles configuration symbols
        hb_mc_coordinate_t tg_id = hb_mc_coordinate (0, 0);
        hb_mc_coordinate_t tg_dim = hb_mc_coordinate (1, 1);
        hb_mc_coordinate_t grid_dim = hb_mc_coordinate (1, 1);
        mesh_foreach_tile(pod->mesh, tile)
        {
                BSG_CUDA_CALL(tile_set_config_symbols(device, pod, tile,
                                                      &default_map,
                                                      pod->mesh->origin,
                                                      tg_id,
                                                      tg_dim,
                                                      grid_dim));

                BSG_CUDA_CALL(tile_unfreeze(device, pod, tile));
        }

        return HB_MC_SUCCESS;
}

/**
 * Initializes a CUDA-Lite program on the manycore on a pod specified.
 * @param[in] device Pointer to device
 * @param[in] name   Device name
 * @param[in] id     Device id
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_program_init(hb_mc_device_t *device,
                                  hb_mc_pod_id_t  pod_id,
                                  const char     *bin_name)
{
        hb_mc_program_options_t popts;
        hb_mc_program_options_default(&popts);

        // call with default opts
        return hb_mc_device_pod_program_init_opts(device, pod_id,
                                                  bin_name,
                                                  &popts);
}

/**
 * Initializes a CUDA-Lite program on the manycore on a pod specified.
 * @param[in] device Pointer to device
 * @param[in] pod    Pod ID
 * @param[in] name   Device name
 * @param[in] id     Device id
 * @param[in] popts  Program options defining program behavior
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_program_init_opts(hb_mc_device_t *device,
                                       hb_mc_pod_id_t  pod_id,
                                       const char     *bin_name,
                                       const hb_mc_program_options_t *popts)
{
        int r = HB_MC_SUCCESS; // return code
        CHECK_POD_ID(device, pod_id);

        // load program data
        unsigned char *bin_data;
        size_t bin_size;

        r = hb_mc_loader_read_program_file(bin_name, &bin_data, &bin_size);
        if (r != HB_MC_SUCCESS)
                return r;

        // call with program data loaded
        hb_mc_program_options_t opts = *popts;
        opts.move_bin_data = 1;  // take ownership of bin_data (don't copy)
        return hb_mc_device_pod_program_init_binary_opts(device, pod_id,
                                                         bin_data, bin_size,
                                                         &opts);
}

/**
 * Initializes a CUDA-Lite program on the manycore on a pod specified.
 * @param[in] device Pointer to device
 * @param[in] name   Device name
 * @param[in] id     Device id
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_program_init_binary(hb_mc_device_t      *device,
                                         hb_mc_pod_id_t       pod_id,
                                         const unsigned char *bin_data,
                                         size_t               bin_size)
{
        hb_mc_program_options_t popts;
        hb_mc_program_options_default(&popts);

        // call with default opts
        return hb_mc_device_pod_program_init_binary_opts(device, pod_id,
                                                         bin_data, bin_size,
                                                         &popts);
}

/**
 * Initializes a CUDA-Lite program on the manycore on a pod specified.
 * @param[in] device Pointer to device
 * @param[in] pod    Pod ID
 * @param[in] name   Device name
 * @param[in] id     Device id
 * @param[in] popts  Program options defining program behavior
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_program_init_binary_opts(hb_mc_device_t       *device,
                                              hb_mc_pod_id_t        pod_id,
                                              const unsigned char  *bin_data,
                                              size_t                bin_size,
                                              const hb_mc_program_options_t *popts)
{
        bsg_pr_dbg("%s: device<%s>: program<%s>\n", __func__, device->name, popts->program_name);
        CHECK_POD_ID(device, pod_id);

        // initialize program on pod
        hb_mc_pod_t *pod = &device->pods[pod_id];

        // initialize mesh
        BSG_CUDA_CALL(hb_mc_device_pod_mesh_init(device, pod, popts));

        // initialize tile groups
        BSG_CUDA_CALL(hb_mc_device_pod_tile_groups_init(device, pod));

        // initialize program
        hb_mc_program_t *program;
        XMALLOC(program);
        XSTRDUP(program->bin_name, popts->program_name);

        // make copy of binary data
        if (popts->move_bin_data) {
                program->bin = bin_data;
                program->bin_size = bin_size;
        } else {
                XMALLOC_N(program->bin, bin_size);
                memcpy(const_cast<unsigned char*>(program->bin), bin_data, bin_size);
                program->bin_size = bin_size;
        }

        // initialize memory allocator
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device->mc);
        BSG_CUDA_CALL(hb_mc_program_allocator_init (cfg, program, popts->alloc_name, popts->alloc_id));

        // set pod program
        pod->program = program;

        // load binary onto all tiles
        BSG_CUDA_CALL(hb_mc_device_pod_program_load(device, pod));

        pod->program_loaded = 1;

        return HB_MC_SUCCESS;
}


/*************************/
/* Pod Interface Cleanup */
/*************************/


/**
 * Cleanup a mesh.
 */
static
int hb_mc_device_pod_mesh_exit(hb_mc_device_t *device,
                               hb_mc_pod_t *pod)
{
        hb_mc_mesh_t *mesh = pod->mesh;
        CHECK_PTR(mesh);

        // Free tile list
        const hb_mc_tile_t *tiles = mesh->tiles;
        CHECK_PTR(tiles);

        free ((void *) tiles);
        mesh->tiles = NULL;

        // free mesh
        free(mesh);
        pod->mesh = NULL;

        return HB_MC_SUCCESS;
}

/**
 * Cleanup tile groups.
 */
__attribute__((warn_unused_result))
static
int hb_mc_device_pod_tile_groups_exit(hb_mc_device_t *device,
                                      hb_mc_pod_t *pod)
{
        // cleanup tile groups
        hb_mc_tile_group_t *tg;
        pod_foreach_tile_group(pod, tg)
        {
                if (tg->status != HB_MC_TILE_GROUP_STATUS_FINISHED)
                        BSG_CUDA_CALL(hb_mc_device_pod_tile_group_exit(device, pod, tg));
        }

        // free tile groups
        free(pod->tile_groups);
        pod->tile_groups = NULL;
        pod->tile_group_capacity = 0;
        pod->num_tile_groups = 0;

        return HB_MC_SUCCESS;
}

/**
 * Performs cleanup for a program loaded onto pod with
 * hb_mc_device_pod_program_init().
 *
 * All memory allocated with hb_mc_device_pod_malloc() is
 * freed.
 *
 * After this function is called, a new program can be
 * initialized on pod using hb_mc_device_pod_program_init().
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_program_finish(hb_mc_device_t *device,
                                    hb_mc_pod_id_t  pod_id)
{
        CHECK_POD_ID(device, pod_id);
        hb_mc_pod_t *pod = &device->pods[pod_id];

        bsg_pr_dbg("%s: calling for pod %d\n",
                   __func__, pod_id);

        if (!pod->program_loaded)
                return HB_MC_SUCCESS;

        // freeze all tiles
        hb_mc_tile_t *tile;
        mesh_foreach_tile(pod->mesh, tile)
        {
                BSG_CUDA_CALL(tile_freeze(device, pod, tile));
        }

        // perform a fence on outstanding host requests
        BSG_MANYCORE_CALL(device->mc, hb_mc_manycore_host_request_fence(device->mc, -1));

        // free resources allocated for program
        hb_mc_program_t *program = pod->program;

        // cleanup tile groups
        BSG_CUDA_CALL(hb_mc_device_pod_tile_groups_exit(device, pod));

        // cleanup mesh
        BSG_CUDA_CALL(hb_mc_device_pod_mesh_exit(device, pod));

        // free allocator
        BSG_CUDA_CALL(hb_mc_program_allocator_exit(program->allocator));

        // free bin data
        free(const_cast<unsigned char*>(program->bin));
        program->bin = NULL;
        program->bin_size = 0;

        // free bin name
        free(const_cast<char*>(program->bin_name));
        program->bin_name = NULL;

        // free program
        free(program);
        pod->program = NULL;

        pod->program_loaded = 0;

        return HB_MC_SUCCESS;
}

/****************************/
/* Pod Interface Allocation */
/****************************/
/**
 * Allocates memory on device's DRAM associated with the input pod
 * hb_mc_device_pod_program_init() should have been called for device and pod
 * before calling this function to set up a memory allocator.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID with a prorgam initialized
 * @parma[in]  size          Size of requested memory
 * @param[out] eva           Eva address of the allocated memory
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_malloc(hb_mc_device_t *device,
                            hb_mc_pod_id_t  pod_id,
                            uint32_t        size,
                            hb_mc_eva_t    *eva)
{
        CHECK_POD_ID(device, pod_id);
        hb_mc_pod_t *pod = &device->pods[pod_id];
        hb_mc_program_t *program = pod->program;
        // check pod has program loaded
        if (program == NULL) {
                bsg_pr_err("%s: no program load on pod: %s\n",
                           __func__,
                           hb_mc_strerror(HB_MC_INVALID));
                return HB_MC_INVALID;
        }

        awsbwhal::MemoryManager *mem_manager = reinterpret_cast<awsbwhal::MemoryManager*>(program->allocator->memory_manager);
        hb_mc_eva_t result = mem_manager->alloc(size);
        if (result == awsbwhal::MemoryManager::mNull) {
                bsg_pr_err("%s: failed to allocate %" PRIu32 " bytes\n",
                           __func__, size);
                return HB_MC_NOMEM;
        }

        *eva = result;
        return HB_MC_SUCCESS;
}

/**
 * Frees memory on device's DRAM associated with the input pod
 * hb_mc_device_pod_program_init() should have been called for device and pod
 * before calling this function to set up a memory allocator.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID with a prorgam initialized
 * @param[out] eva           Eva address of the memory to be freed
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_free(hb_mc_device_t *device,
                          hb_mc_pod_id_t  pod_id,
                          hb_mc_eva_t     eva)
{
        CHECK_POD_ID(device, pod_id);
        hb_mc_pod_t *pod = &device->pods[pod_id];
        hb_mc_program_t *program = pod->program;
        // check pod has program loaded
        if (program == NULL) {
                bsg_pr_err("%s: no program load on pod %d: %s\n",
                           __func__,
                           pod_id,
                           hb_mc_strerror(HB_MC_INVALID));
                return HB_MC_INVALID;
        }

        awsbwhal::MemoryManager *mem_manager = reinterpret_cast<awsbwhal::MemoryManager*>(program->allocator->memory_manager);
        mem_manager->free(eva);
        return HB_MC_SUCCESS;
}

/*******************************/
/* Pod Interface Data Movement */
/*******************************/
/**
 * Copies a buffer from src on the host/pod DRAM to dst on pod DRAM/host.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @parma[in]  src           EVA address of source to be copied from
 * @parma[in]  dst           EVA address of destination to be copied into
 * @param[in]  name          EVA address of dst
 * @param[in]  count         Size of buffer (number of bytes) to be copied
 * @param[in]  hb_mc_memcpy_kind         Direction of copy (HB_MC_MEMCPY_TO_DEVICE / HB_MC_MEMCPY_TO_HOST)
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_memcpy(hb_mc_device_t *device,
                            hb_mc_pod_id_t pod_id,
                            void *dst,
                            const void *src,
                            uint32_t bytes,
                            enum hb_mc_memcpy_kind kind)
{
        CHECK_POD_ID(device, pod_id);

        size_t sz = static_cast<size_t>(bytes);

        if (kind == HB_MC_MEMCPY_TO_DEVICE) {
                hb_mc_eva_t dst_eva = (hb_mc_eva_t) reinterpret_cast<uintptr_t>(dst);
                return hb_mc_device_pod_memcpy_to_device(device, pod_id, dst_eva, src, sz);
        } else if (kind == HB_MC_MEMCPY_TO_HOST) {
                hb_mc_eva_t src_eva = (hb_mc_eva_t) reinterpret_cast<uintptr_t>(src);
                return hb_mc_device_pod_memcpy_to_host(device, pod_id, dst, src_eva, sz);
        } else {
                bsg_pr_err("%s: invalid copy type. Copy type can be one of "
                           "HB_MC_MEMCPY_TO_DEVICE or HB_MC_MEMCPY_TO_HOST.\n",
                           __func__);
                return HB_MC_INVALID;
        }
}

/**
 * Copies a buffer from src on the host to dst on pod DRAM.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @parma[in]  daddr         EVA address of destination to be copied into
 * @parma[in]  haddr         Host address of source to be copied from
 * @param[in]  bytes         Size of buffer to be copied
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_memcpy_to_device(hb_mc_device_t *device,
                                      hb_mc_pod_id_t pod_id,
                                      hb_mc_eva_t daddr,
                                      const void *haddr,
                                      uint32_t bytes)
{
        CHECK_POD_ID(device, pod_id);

        hb_mc_pod_t *pod = &device->pods[pod_id];

        BSG_MANYCORE_CALL(device->mc,
                          hb_mc_manycore_eva_write(device->mc,
                                                   &default_map,
                                                   &pod->mesh->origin,
                                                   &daddr, haddr, bytes));

        return HB_MC_SUCCESS;
}

/**
 * Copies a buffer from src on pod DRAM to dst on host.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @parma[in]  daddr         EVA address of destination to be copied into
 * @parma[in]  haddr         Host address of source to be copied from
 * @param[in]  bytes         Size of buffer to be copied
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_memcpy_to_host(hb_mc_device_t *device,
                                    hb_mc_pod_id_t pod_id,
                                    void *haddr,
                                    hb_mc_eva_t daddr,
                                    uint32_t bytes)
{
        CHECK_POD_ID(device, pod_id);

        hb_mc_pod_t *pod = &device->pods[pod_id];

        BSG_CUDA_CALL(hb_mc_manycore_eva_read(device->mc,
                                              &default_map,
                                              &pod->mesh->origin,
                                              &daddr, haddr, bytes));

        return HB_MC_SUCCESS;
}

/**
 * Sets memory to a given value starting from an address in pod's DRAM.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @parma[in]  eva           EVA address of destination
 * @param[in]  val           Value to be written out
 * @param[in]  sz            The number of bytes to write into device DRAM
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_memset (hb_mc_device_t *device,
                             hb_mc_pod_id_t pod_id,
                             hb_mc_eva_t  eva,
                             uint8_t data,
                             size_t sz)
{
        CHECK_POD_ID(device, pod_id);

        hb_mc_pod_t *pod = &device->pods[pod_id];

        BSG_CUDA_CALL(hb_mc_manycore_eva_memset (device->mc,
                                                 &default_map,
                                                 &pod->mesh->origin,
                                                 &eva,
                                                 data,
                                                 sz));
        return HB_MC_SUCCESS;
}

/***********************************/
/* Pod Interface Execution Control */
/***********************************/
/**
 * Initialize a tile group
 */
static int hb_mc_device_pod_tile_group_init(hb_mc_device_t* device,
                                            hb_mc_pod_t *pod,
                                            hb_mc_tile_group_t *tg,
                                            grid_id_t grid_id,
                                            hb_mc_coordinate_t tg_id,
                                            hb_mc_dimension_t grid_dim,
                                            hb_mc_dimension_t dim,
                                            hb_mc_kernel_t *kernel)
{
        tg->dim = dim;
        tg->origin = pod->mesh->origin;
        tg->id = tg_id;
        tg->grid_id = grid_id;
        tg->grid_dim = grid_dim;
        tg->status = HB_MC_TILE_GROUP_STATUS_INITIALIZED;

        hb_mc_coordinate_t host = hb_mc_manycore_get_host_coordinate(device->mc);
        tg->finish_signal_npa = hb_mc_npa(host, hb_mc_tile_group_get_finish_signal_addr(tg));

        // initialize address map
        XMALLOC(tg->map);
        BSG_CUDA_CALL(hb_mc_origin_eva_map_init(tg->map, tg->origin));

        // set kernel and increment kernel reference count
        tg->kernel = kernel;
        kernel->refcount += 1;
        return HB_MC_SUCCESS;
}
/**
 * Cleanup a tile group
 */
static int hb_mc_device_pod_tile_group_exit(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_group_t *tg)
{

        // Free the memory location in the device that holds the list of
        // arguments of tile group's kernel
        hb_mc_pod_id_t pod_id = hb_mc_device_pod_to_pod_id(device, pod);
        BSG_CUDA_CALL(hb_mc_device_pod_free(device, pod_id, tg->argv_eva));
        BSG_CUDA_CALL(hb_mc_device_pod_free(device, pod_id, tg->barcfg_eva));

        // release tile gorup resources
        tg->dim = HB_MC_DIMENSION(0,0);
        tg->origin = pod->mesh->origin;
        tg->id = hb_mc_coordinate(0,0);
        tg->grid_id = 0;
        tg->status = HB_MC_TILE_GROUP_STATUS_FINISHED;

        // free the map
        BSG_CUDA_CALL(hb_mc_origin_eva_map_exit(tg->map));
        free(tg->map);

        // decrement the kernel reference count and free if needed
        tg->kernel->refcount -= 1;
        if (tg->kernel->refcount == 0)
                BSG_CUDA_CALL(kernel_exit(tg->kernel));

        tg->kernel = NULL;
        return HB_MC_SUCCESS;
}

/**
 * Enqueue a tile group
 */
__attribute__((warn_unused_result))
static int hb_mc_device_pod_tile_group_kernel_enqueue (hb_mc_device_t* device,
                                                       hb_mc_pod_t *pod,
                                                       grid_id_t grid_id,
                                                       hb_mc_coordinate_t tg_id,
                                                       hb_mc_dimension_t grid_dim,
                                                       hb_mc_dimension_t dim,
                                                       hb_mc_kernel_t *kernel)
{
        // increaase tile group capacity if necessary
        int cap = pod->tile_group_capacity;
        int num = pod->num_tile_groups;
        if (cap == num) {
                XREALLOC(pod->tile_groups, cap * 2);
                pod->tile_group_capacity *= 2;
        }

        // initialize tile group
        hb_mc_tile_group_t* tg = &pod->tile_groups[pod->num_tile_groups];
        BSG_CUDA_CALL(hb_mc_device_pod_tile_group_init(device, pod, tg,
                                                       grid_id, tg_id, grid_dim, dim, kernel));

        // incremement the number of tile groups
        pod->num_tile_groups += 1;

        bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) initialized.\n",
                   __func__,
                   tg->grid_id,
                   hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id)) ;

        return HB_MC_SUCCESS;
}

/**
 * Enqueues and schedules a kernel to be run on a pod
 * Takes the grid size, tile group dimensions, kernel name, argc,
 * argv* and adds a kernel invocation to a queue.
 *
 * When the kernel is invoked, it will execute with tile groups of size
 * given by tg_dim. The number of tile groups is given by the dimension
 * specified in grid_dim.
 *
 * The kernels are not launched with this call.
 * See hb_mc_device_pod_kernels_execute() to launch enqueued kernels.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
 * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
 * @param[in]  name          Kernel name to be executed on tile groups in grid
 * @param[in]  argc          Number of input arguments to kernel
 * @param[in]  argv          List of input arguments to kernel
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_kernel_enqueue(hb_mc_device_t    *device,
                                    hb_mc_pod_id_t     pod_id,
                                    hb_mc_dimension_t  grid_dim,
                                    hb_mc_dimension_t  tg_dim,
                                    const char* name,
                                    uint32_t argc,
                                    const uint32_t *argv)
{
        CHECK_POD_ID(device, pod_id);
        CHECK_PTR(device->pods);

        hb_mc_pod_t *pod = &device->pods[pod_id];

        bsg_pr_dbg("%s: device<%s>: program<%s>: calling\n",
                   __func__, device->name, pod->program->bin_name);

        // create a kernel
        hb_mc_kernel_t *kernel;
        XMALLOC(kernel);
        BSG_CUDA_CALL(kernel_init(kernel, name, argc, argv));

        // add all tile groups
        hb_mc_coordinate_t tg_id;
        foreach_coordinate(tg_id, HB_MC_COORDINATE(0,0), grid_dim)
        {
                BSG_CUDA_CALL(hb_mc_device_pod_tile_group_kernel_enqueue(device, pod, pod->num_grids, tg_id, grid_dim, tg_dim, kernel));
        }

        pod->num_grids++;
        return HB_MC_SUCCESS;
}

/**
 * Check if all tile groups have completed
 */
static
int hb_mc_device_pod_all_tile_groups_finished(hb_mc_device_t *device, hb_mc_pod_t *pod)
{
        hb_mc_tile_group_t *tile_group;
        pod_foreach_tile_group(pod, tile_group)
        {
                if (tile_group->status != HB_MC_TILE_GROUP_STATUS_FINISHED)
                        return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

/**
 * Allocate a group of free tiles and store their origin in tile_group
 */
__attribute__((warn_unused_result))
static
int hb_mc_device_pod_tile_group_allocate_tiles(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_group_t *tile_group)
{


        bsg_pr_dbg("%s: device<%s>: program<%s>: calling\n",
                   __func__, device->name, pod->program->bin_name);

        // calculate boundary condition
        hb_mc_dimension_t origin_boundary;
        BSG_CUDA_CALL(hb_mc_coordinate_sub_safe(pod->mesh->dim, tile_group->dim, &origin_boundary));
        origin_boundary = hb_mc_coordinate_add(origin_boundary, hb_mc_dimension(1,1));

        // scan for contiguous group of free tile groups
        hb_mc_coordinate_t origin;
        int tiles_are_free = 0;

#if defined (DEBUG)
        char origin_str[256];
        char boundary_str[256];
#endif

        bsg_pr_dbg("%s: scanning orig:%s  dim:%s\n",
                   __func__,
                   hb_mc_coordinate_to_string(pod->mesh->origin, origin_str, sizeof(origin_str)),
                   hb_mc_coordinate_to_string(origin_boundary, boundary_str, sizeof(boundary_str)));

        foreach_coordinate(origin, pod->mesh->origin, origin_boundary)
        {
                // check if this group of tiles is free
                // this group is free if the group of tiles about the origin matching
                // the required tile group size are all free
                tiles_are_free = 1;
                hb_mc_coordinate_t xy;
                foreach_coordinate(xy, origin, tile_group->dim)
                {
                        hb_mc_idx_t tile_id = hb_mc_get_tile_id(pod->mesh->origin, pod->mesh->dim, xy);
                        if (pod->mesh->tiles[tile_id].status != HB_MC_TILE_STATUS_FREE) {
                                tiles_are_free = 0;
                                break;
                        }
                }


                bsg_pr_dbg("%s: %s %s\n",
                           __func__,
                           hb_mc_coordinate_to_string(origin, origin_str, sizeof(origin_str)),
                           tiles_are_free ? "free" : "not free");

                if (!tiles_are_free)
                        continue;

                // these tiles are free; set the origin as the tile groups origin
                tile_group->origin = origin;

                // initialize free group of tiles
                foreach_coordinate(xy, tile_group->origin, tile_group->dim)
                {
                        hb_mc_idx_t tile_id = hb_mc_get_tile_id(pod->mesh->origin, pod->mesh->dim, xy);

                        // initialize eva map to support tile group addressing
                        BSG_CUDA_CALL(hb_mc_origin_eva_map_exit(tile_group->map));
                        BSG_CUDA_CALL(hb_mc_origin_eva_map_init(tile_group->map, origin));

                        // set bookkeeping fields
                        hb_mc_tile_t *tile = &pod->mesh->tiles[tile_id];
                        tile->origin = origin;
                        tile->tile_group_id = tile_group->id;
                        tile->status = HB_MC_TILE_STATUS_BUSY;

                        // set configuration symbols
                        BSG_CUDA_CALL(tile_set_config_symbols(device, pod, tile,
                                                              tile_group->map,
                                                              tile_group->origin,
                                                              tile_group->id,
                                                              tile_group->dim,
                                                              tile_group->grid_dim));
                }
                break; // done
        }

        if (!tiles_are_free)
                return HB_MC_NOTFOUND;

        tile_group->status = HB_MC_TILE_GROUP_STATUS_ALLOCATED;
        return HB_MC_SUCCESS;
}

__attribute__((warn_unused_result))
static
int hb_mc_device_pod_tile_group_deallocate_tiles(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_group_t *tg)
{
        hb_mc_idx_t tile_id;
        hb_mc_coordinate_t xy;

        bsg_pr_dbg("%s: device<%s>: program<%s>: calling\n",
                   __func__, device->name, pod->program->bin_name);

        // make the tiles as free and reset origin/tile group id
        foreach_coordinate(xy, tg->origin, tg->dim)
        {
                tile_id = hb_mc_get_tile_id (pod->mesh->origin, pod->mesh->dim, xy);
                hb_mc_tile_t *tile = &pod->mesh->tiles[tile_id];

                tile->origin = pod->mesh->origin;
                tile->tile_group_id = hb_mc_coordinate(0, 0);
                tile->status = HB_MC_TILE_STATUS_FREE;
        }

        bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) de-allocated at origin (%d,%d).\n",
                   __func__,
                   tg->grid_id,
                   hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
                   hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));


        return HB_MC_SUCCESS;
}

/**
 * Initialize the array of CSR values for the hw barrier.
 * This function checks if the barrier is used in the kernel, and if so an array is allocated
 * and initialized.
 */
static int hb_mc_device_pod_tile_group_barrier_init(hb_mc_device_t *device
                                                    , hb_mc_pod_t *pod
                                                    , hb_mc_tile_group_t *tg)
{
        hb_mc_kernel_t *kernel = tg->kernel;
        bsg_pr_dbg("%s: device<%s>: program<%s>: Initializing hardware barrier array for kernel '%s'\n"
                   , __func__
                   , device->name
                   , pod->program->bin_name
                   , kernel->name);

        // check that the barrier is used
        // to do this, look for a symbol "__cuda_barrier_cfg"
        int err;
        hb_mc_eva_t barr_config_ptr;
        err = hb_mc_loader_symbol_to_eva(pod->program->bin
                                         , pod->program->bin_size
                                         , "__cuda_barrier_cfg"
                                         , &barr_config_ptr);

        // if not found, no barrier initialization
        if (err == HB_MC_NOTFOUND) {
                tg->barcfg_eva = 0;
                return HB_MC_SUCCESS;
        }

        // found the barrier pointer
        // allocate an array for csr values
        hb_mc_eva_t barcfg_eva;
        hb_mc_pod_id_t pod_id = hb_mc_device_pod_to_pod_id(device, pod);
        BSG_CUDA_CALL(hb_mc_device_pod_malloc(device
                                              , pod_id
                                              , (1 + tg->dim.x * tg->dim.y) * sizeof(int)
                                              , &barcfg_eva));

        // initialize barcfg
        int barcfg [tg->dim.x * tg->dim.y + 1];
        hb_mc_coordinate_t cord, og = hb_mc_coordinate(0,0);
        foreach_coordinate(cord, og, tg->dim) {
                int id = cord.y * tg->dim.x + cord.x;

                // compute for id
                barcfg[1+id] = hb_mc_hw_barrier_csr_val(&device->mc->config, cord.x, cord.y, tg->dim.x, tg->dim.y);
        }
        // word zero holds the amoadd barrier lock
        barcfg[0] = 0;

        // copy csr val vector to device
        BSG_CUDA_CALL(hb_mc_device_pod_memcpy_to_device(device, pod_id, barcfg_eva, barcfg, sizeof(barcfg)));

        // save so we can free later
        tg->barcfg_eva = barcfg_eva;

        return HB_MC_SUCCESS;       
}

__attribute__((warn_unused_result))
static
int hb_mc_device_pod_tile_group_launch(hb_mc_device_t *device, hb_mc_pod_t *pod, hb_mc_tile_group_t *tile_group)
{
        hb_mc_kernel_t *kernel = tile_group->kernel;
        bsg_pr_dbg("%s: device<%s>: program<%s>: Launching tile group running kernel = '%s'\n",
                   __func__, device->name, pod->program->bin_name, kernel->name);

        // initialize argv
        // allocate argv
        hb_mc_eva_t argv_addr;
        hb_mc_pod_id_t pod_id = hb_mc_device_pod_to_pod_id(device, pod);
        BSG_CUDA_CALL(hb_mc_device_pod_malloc(device, pod_id, kernel->argc * sizeof(*(kernel->argv)), &argv_addr));
        tile_group->argv_eva = argv_addr;

        // copy argv over
        BSG_CUDA_CALL(hb_mc_device_pod_memcpy_to_device(device, pod_id,
                                                        tile_group->argv_eva,
                                                        &kernel->argv[0],
                                                        kernel->argc * sizeof(*(kernel->argv))));

        // initialize hw barrier array
        BSG_CUDA_CALL(hb_mc_device_pod_tile_group_barrier_init(device, pod, tile_group));

        // find kernel
        hb_mc_eva_t kernel_addr;
        BSG_CUDA_CALL(hb_mc_loader_symbol_to_eva(pod->program->bin, pod->program->bin_size, kernel->name, &kernel_addr));


        hb_mc_coordinate_t coord;
        foreach_coordinate(coord, tile_group->origin, tile_group->dim)
        {
                hb_mc_idx_t tile_id = hb_mc_get_tile_id(pod->mesh->origin, pod->mesh->dim, coord);
                hb_mc_tile_t *tile = &pod->mesh->tiles[tile_id];
                // this will wake the tile up
                BSG_CUDA_CALL(tile_set_runtime_symbols(device, pod, tile, tile_group,
                                                       kernel->argc, kernel_addr));
        }

        // make tile group as launched
        tile_group->status = HB_MC_TILE_GROUP_STATUS_LAUNCHED;

        return HB_MC_SUCCESS;
}

// forward declaration
static
int hb_mc_device_podv_wait_for_tile_group_finish_any(hb_mc_device_t *device,
                                                     hb_mc_pod_id_t *podv,
                                                     int podc,
                                                     hb_mc_pod_id_t *pod_done);

/**
 * Wait for a tile group to complete for a pod.
 */
static
int hb_mc_device_pod_wait_for_tile_group_finish_any(hb_mc_device_t *device, hb_mc_pod_t *pod)
{
        hb_mc_pod_id_t pid, pid_done;
        pid = hb_mc_device_pod_to_pod_id(device, pod);
        return hb_mc_device_podv_wait_for_tile_group_finish_any(device,
                                                                &pid, 1,
                                                                &pid_done);
}

/**
 * Try to launch as many tile groups as possible in pod
 */
static
int hb_mc_device_pod_try_launch_tile_groups(hb_mc_device_t *device,
                                            hb_mc_pod_t *pod)

{
        int r;
        hb_mc_tile_group_t *tg;
        hb_mc_dimension_t last_failed = hb_mc_dimension(0,0);

        // scan for ready tile groups
        pod_foreach_tile_group(pod, tg)
        {
                // only look at ready tile groups
                if (tg->status != HB_MC_TILE_GROUP_STATUS_INITIALIZED)
                        continue;

                // skip if we know this shape fails
                if (last_failed.x == tg->dim.x &&
                    last_failed.y == tg->dim.y)
                        continue;

                // keep going if we can't allocate
                r = hb_mc_device_pod_tile_group_allocate_tiles(device, pod, tg);
                if (r != HB_MC_SUCCESS) {
                        // mark this shape as the last failed
                        last_failed = tg->dim;
                        continue;
                }

                // launch the tile tile group
                BSG_CUDA_CALL(hb_mc_device_pod_tile_group_launch(device, pod, tg));
        }

        return HB_MC_SUCCESS;
}

/**
 * Launches all kernel invocations enqueued on pod.
 * These kernel invocations are enqueued by
 * hb_mc_device_pod_kernel_enqueue().
 *
 * This function blocks until all kernels have been invoked
 * and completed.
 * @param[in]  device        Pointer to device
 * @param[in]  pod           Pod ID
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pod_kernels_execute(hb_mc_device_t *device,
                                     hb_mc_pod_id_t pod_id)
{
        CHECK_POD_ID(device, pod_id);
        hb_mc_pod_t *pod = &device->pods[pod_id];
        int r;

        bsg_pr_dbg("%s: device<%s>: program<%s>: calling\n",
                   __func__, device->name, pod->program->bin_name);

        while (hb_mc_device_pod_all_tile_groups_finished(device, pod) != HB_MC_SUCCESS) {
                // try launching as many tile groups as possible
                BSG_CUDA_CALL(hb_mc_device_pod_try_launch_tile_groups(device, pod));

                // wait for any tile group to complete
                BSG_CUDA_CALL(hb_mc_device_pod_wait_for_tile_group_finish_any(device, pod));
        }

        return HB_MC_SUCCESS;
}

/**
 * Returns true if all pods in podv have all tile-groups finished.
 */
static
int hb_mc_device_podv_all_tile_groups_finished(hb_mc_device_t *device,
                                               hb_mc_pod_id_t *podv,
                                               int podc)
{
        for (int podi = 0; podi < podc; podi++)
        {
                hb_mc_pod_t *pod = &device->pods[podv[podi]];
                if (hb_mc_device_pod_all_tile_groups_finished(device, pod) != HB_MC_SUCCESS) {
                        return HB_MC_FAIL;
                }
        }
        return HB_MC_SUCCESS;
}

/**
 * Try to launch as many tile groups as possible in all pods in podv
 */
static
int hb_mc_device_podv_try_launch_tile_groups(hb_mc_device_t *device,
                                             hb_mc_pod_id_t *podv,
                                             int podc)
{
        // try launching as many tile groups as possible on all pods
        for (int podi = 0; podi < podc; podi++)
        {
                hb_mc_pod_t *pod = &device->pods[podv[podi]];
                BSG_CUDA_CALL(hb_mc_device_pod_try_launch_tile_groups(device, pod));
        }
        return HB_MC_SUCCESS;
}

/**
 * Wait for any tile group to complete. Cleanup and release that tile groups resources.
 * @return pod_done  The pod on which a tile-group just completed
 */
static
int hb_mc_device_podv_wait_for_tile_group_finish_any(hb_mc_device_t *device,
                                                     hb_mc_pod_id_t *podv,
                                                     int podc,
                                                     hb_mc_pod_id_t *pod_done)
{
        bsg_pr_dbg("%s: calling\n", __func__);

        while (true) {
                hb_mc_request_packet_t rqst;

                // perform a blocking read from the request fifo
                BSG_CUDA_CALL(hb_mc_manycore_request_rx(device->mc, &rqst, -1));

                #ifdef DEBUG
                char pkt_str[256];
                hb_mc_request_packet_to_string(&rqst, pkt_str, sizeof(pkt_str));
                bsg_pr_dbg("%s: received packet %s\n",
                           __func__,
                           pkt_str);
                #endif
                // request packet read
                // is it a finish packet?
                if (hb_mc_request_packet_get_data(&rqst) != HB_MC_CUDA_FINISH_SIGNAL_VAL) {
                        bsg_pr_dbg("%s: not a finish packet\n", __func__);
                        continue;
                }

                // identify the pod
                hb_mc_coordinate_t src =
                        hb_mc_coordinate(hb_mc_request_packet_get_x_src(&rqst),
                                         hb_mc_request_packet_get_y_src(&rqst));

                hb_mc_coordinate_t podco = hb_mc_config_pod(&device->mc->config, src);
                hb_mc_pod_id_t pid = hb_mc_coordinate_to_index(podco, device->mc->config.pods);
                hb_mc_pod_t *pod = &device->pods[pid];

                // find the tile group with matching origin in pod
                hb_mc_tile_group_t *tg;
                pod_foreach_tile_group(pod, tg)
                {
                        // only look for launched tile groups
                        if (tg->status != HB_MC_TILE_GROUP_STATUS_LAUNCHED) {
                                continue;
                        }

                        // origin matches?
                        if (!(tg->origin.x == src.x && tg->origin.y == src.y))
                                continue;

                        // finish signal epa matches?
                        if (hb_mc_request_packet_get_epa(&rqst)
                            != hb_mc_npa_get_epa(&tg->finish_signal_npa))
                                continue;

                        #ifdef DEBUG
                        bsg_pr_dbg("%s: received finish packet from (%d,%d)\n",
                                   __func__, tg->origin.x, tg->origin.y);
                        #endif
                        // this is the matching tile group
                        // deallocate tiles
                        BSG_CUDA_CALL(hb_mc_device_pod_tile_group_deallocate_tiles(device, pod, tg));

                        // cleanup tile group
                        BSG_CUDA_CALL(hb_mc_device_pod_tile_group_exit(device, pod, tg));

                        // mark this pod as having completed a tile-group
                        *pod_done = pid;
                        return HB_MC_SUCCESS;
                }

                bsg_pr_dbg("%s: packet received with finished signal "
                           "value but no matching tile-group",
                           __func__);
        }
}

/**
 * Launches all kernel invocations enqueued on pods.
 * These kernel invocations are enqueued by
 * hb_mc_device_pod_kernel_enqueue().
 *
 * This function blocks until all kernels have been invoked
 * and completed.
 * @param[in]  device        Pointer to device
 * @param[in]  podv          Vector of Pod IDs
 * @param[in]  podc          Number of Pod IDs
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_podv_kernels_execute(hb_mc_device_t *device,
                                      hb_mc_pod_id_t *podv,
                                      int podc)
{
        /* launch as many tile groups as possible on all pods */
        BSG_CUDA_CALL(hb_mc_device_podv_try_launch_tile_groups(device, podv, podc));

        /* until all tile groups have completed */
        while (hb_mc_device_podv_all_tile_groups_finished(device, podv, podc) != HB_MC_SUCCESS)
        {
                /* wait for any tile group to finish on any pod */
                hb_mc_pod_id_t pod;
                BSG_CUDA_CALL(hb_mc_device_podv_wait_for_tile_group_finish_any(device, podv, podc,
                                                                               &pod));

                /* try launching launching tile groups on pod with most recent completion */
                BSG_CUDA_CALL(hb_mc_device_pod_try_launch_tile_groups(device, &device->pods[pod]));
        }
        return HB_MC_SUCCESS;
}

/**
 * Launches all kernel invocations enqueued on pods.
 * These kernel invocations are enqueued by
 * hb_mc_device_pod_kernel_enqueue().
 *
 * This function blocks until all kernels have been invoked
 * and completed.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_pods_kernels_execute(hb_mc_device_t *device)
{
        hb_mc_pod_id_t podv[device->num_pods];
        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(device, pod)
        {
                podv[pod]=pod;
        }
        return hb_mc_device_podv_kernels_execute(device, podv, device->num_pods);
}


/********************/
/* Legacy Interface */
/********************/


/**
 * Takes in a buffer containing the binary and its size,
 * freezes tiles, loads program binary into all tiles and into dram,
 * and sets the symbols and registers for each tile.
 * @param[in]  device        Pointer to device
 * @parma[in]  bin_name      Name of binary elf file
 * @param[in]  bin_data      Buffer containing binary
 * @param[in]  bin_size      Size of the binary to be loaded onto device
 * @param[in]  id            Id of program's memory allocator
 * @param[in]  alloc_name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_program_init_binary (hb_mc_device_t *device,
                                      const char *bin_name,
                                      const unsigned char *bin_data,
                                      size_t bin_size,
                                      const char *alloc_name,
                                      hb_mc_allocator_id_t id)
{
        hb_mc_program_options_t opts;
        hb_mc_program_options_default(&opts);
        opts.alloc_name = alloc_name;
        opts.alloc_id = id;
        opts.program_name = bin_name;
        return hb_mc_device_pod_program_init_binary_opts(device, device->default_pod_id, bin_data, bin_size, &opts);
}





/**
 * Takes in a binary name, loads the binary from file onto a buffer,
 * freezes tiles, loads program binary into all tiles and into dram,
 * and sets the symbols and registers for each tile.
 * @param[in]  device        Pointer to device
 * @parma[in]  bin_name      Name of binary elf file
 * @param[in]  id            Id of program's memory allocator
 * @param[in]  alloc_name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_program_init (hb_mc_device_t *device,
                               const char *bin_name,
                               const char *alloc_name,
                               hb_mc_allocator_id_t id)
{
        hb_mc_program_options_t opts;
        hb_mc_program_options_default(&opts);
        opts.alloc_name = alloc_name;
        opts.alloc_id = id;
        opts.program_name = bin_name;
        return hb_mc_device_pod_program_init_opts(device, device->default_pod_id, bin_name, &opts);
}



/**
 * Allocates memory on device DRAM
 * hb_mc_device_program_init() or hb_mc_device_program_init_binary() should
 * have been called before calling this function to set up a memory allocator.
 * @param[in]  device        Pointer to device
 * @parma[in]  size          Size of requested memory
 * @param[out] eva           Eva address of the allocated memory
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_malloc (hb_mc_device_t *device, uint32_t size, hb_mc_eva_t *eva)
{
        return hb_mc_device_pod_malloc(device, device->default_pod_id, size, eva);
}





/**
 * Copies a buffer from src on the host/device DRAM to dst on device DRAM/host.
 * @param[in]  device        Pointer to device
 * @parma[in]  src           EVA address of source to be copied from
 * @parma[in]  dst           EVA address of destination to be copied into
 * @param[in]  name          EVA address of dst
 * @param[in]  count         Size of buffer (number of bytes) to be copied
 * @param[in]  hb_mc_memcpy_kind         Direction of copy (HB_MC_MEMCPY_TO_DEVICE / HB_MC_MEMCPY_TO_HOST)
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_memcpy (hb_mc_device_t *device,
                         void *dst,
                         const void *src,
                         uint32_t count,
                         enum hb_mc_memcpy_kind kind)
{
        return hb_mc_device_pod_memcpy(device, device->default_pod_id,
                                       dst, src, count, kind);
}


/**
 * Copies a buffer from src on the host to dst on device DRAM.
 * @param[in]  device        Pointer to device
 * @parma[in]  daddr         EVA address of destination to be copied into
 * @parma[in]  haddr         Host address of source to be copied from
 * @param[in]  bytes         Size of buffer to be copied
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_memcpy_to_device(hb_mc_device_t *device,
                                  hb_mc_eva_t daddr,
                                  const void *haddr,
                                  uint32_t bytes)
{
        return hb_mc_device_pod_memcpy_to_device(device, device->default_pod_id,
                                             daddr, haddr, bytes);
}

/**
 * Copies a buffer from src on device DRAM to dst on device the host.
 * @param[in]  device        Pointer to device
 * @parma[in]  haddr         Host address of source to be copied into
 * @parma[in]  daddr         EVA address of destination to be copied from
 * @param[in]  bytes         Size of buffer to be copied
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_memcpy_to_host(hb_mc_device_t *device,
                                void       *haddr,
                                hb_mc_eva_t daddr,
                                uint32_t bytes)
{
        return hb_mc_device_pod_memcpy_to_host(device, device->default_pod_id,
                                           haddr, daddr, bytes);
}


/**
 * Sets memory to a give value starting from an address in device's DRAM.
 * @param[in]  device        Pointer to device
 * @parma[in]  eva           EVA address of destination
 * @param[in]  val           Value to be written out
 * @param[in]  sz            The number of bytes to write into device DRAM
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_memset (hb_mc_device_t *device,
                         const hb_mc_eva_t *eva,
                         uint8_t data,
                         size_t sz)
{
        return hb_mc_device_pod_memset(device, device->default_pod_id, *eva, data, sz);
}


int hb_mc_device_pod_dma_to_device(hb_mc_device_t *device, hb_mc_pod_id_t pod_id, const hb_mc_dma_htod_t *jobs, size_t count)
{
        int err;
        CHECK_POD_ID(device, pod_id);

        if (!hb_mc_manycore_supports_dma_read(device->mc))
                return HB_MC_NOIMPL;

        hb_mc_pod_t *pod = &device->pods[pod_id];

        // flush cache
        err = hb_mc_manycore_pod_flush_vcache(device->mc, pod->pod_coord);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to flush victim cache: %s\n",
                           __func__,
                           hb_mc_strerror(err));
                return err;
        }

        // for each job...
        for (size_t i = 0; i < count; i++) {

                // perform dma write
                const hb_mc_dma_htod_t *dma = &jobs[i];
                err = hb_mc_manycore_eva_write_dma
                        (device->mc,
                         &default_map,
                         &pod->mesh->origin,
                         &dma->d_addr,
                         dma->h_addr,
                         dma->size);

                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to perform DMA write to 0x%" PRIx32 ": %s\n",
                                   __func__,
                                   dma->d_addr,
                                   hb_mc_strerror(err));
                        return err;
                }
        }

        // invalidate cache
        err = hb_mc_manycore_pod_invalidate_vcache(device->mc, pod->pod_coord);
        if (err != HB_MC_SUCCESS) {
                return err;
        }

        return HB_MC_SUCCESS;
}


int hb_mc_device_pod_dma_to_host(hb_mc_device_t *device, hb_mc_pod_id_t pod_id, const hb_mc_dma_dtoh_t *jobs, size_t count)
{
        int err;
        CHECK_POD_ID(device, pod_id);

        if (!hb_mc_manycore_supports_dma_read(device->mc))
                return HB_MC_NOIMPL;

        // flush cache
        hb_mc_pod_t *pod = &device->pods[pod_id];
        err = hb_mc_manycore_pod_flush_vcache(device->mc, pod->pod_coord);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to flush victim cache: %s\n",
                           __func__,
                           hb_mc_strerror(err));
                return err;
        }

        // for each job...
        for (size_t i = 0; i < count; i++) {

                // perform dma read
                const hb_mc_dma_dtoh_t *dma = &jobs[i];
                err = hb_mc_manycore_eva_read_dma
                        (device->mc,
                         &default_map,
                         &pod->mesh->origin,
                         &dma->d_addr,
                         dma->h_addr,
                         dma->size);

                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to perform DMA read from 0x%" PRIx32 ": %s\n",
                                   __func__,
                                   dma->d_addr,
                                   hb_mc_strerror(err));
                        return err;
                }
        }

        return HB_MC_SUCCESS;
}


/**
 * Copy data using DMA from the host to the device.
 * @param[in] device  Pointer to device
 * @param[in] jobs    Vector of host-to-device DMA jobs
 * @param[in] count   Number of host-to-device jobs
 */
__attribute__((weak))
int hb_mc_device_dma_to_device(hb_mc_device_t *device, const hb_mc_dma_htod_t *jobs, size_t count)
{
        return hb_mc_device_pod_dma_to_device(device, device->default_pod_id, jobs, count);
}

/**
 * Copy data using DMA from the host to the device.
 * @param[in] device  Pointer to device
 * @param[in] jobs    Vector of device-to-host DMA jobs
 * @param[in] count   Number of device-to-host jobs
 */
__attribute__((weak))
int hb_mc_device_dma_to_host(hb_mc_device_t *device, const hb_mc_dma_dtoh_t *jobs, size_t count)
{
        return hb_mc_device_pod_dma_to_host(device, device->default_pod_id, jobs, count);
}

/**
 * Frees memory on device DRAM
 * hb_mc_device_program_init() or hb_mc_device_program_init_binary() should
 * have been called before calling this function to set up a memory allocator.
 * @param[in]  device        Pointer to device
 * @param[out] eva           Eva address of the memory to be freed
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_free (hb_mc_device_t *device, hb_mc_eva_t eva)
{
        return hb_mc_device_pod_free(device, device->default_pod_id, eva);
}





/**
 * Enqueues and schedules a kernel to be run on device
 * Takes the grid size, tile group dimensions, kernel name, argc,
 * argv* and the finish signal address, calls hb_mc_tile_group_enqueue
 * to initialize all tile groups for grid.
 * @param[in]  device        Pointer to device
 * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
 * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
 * @param[in]  name          Kernel name to be executed on tile groups in grid
 * @param[in]  argc          Number of input arguments to kernel
 * @param[in]  argv          List of input arguments to kernel
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_kernel_enqueue (hb_mc_device_t *device,
                          hb_mc_dimension_t grid_dim,
                          hb_mc_dimension_t tg_dim,
                          const char *name,
                          const uint32_t argc,
                          const uint32_t *argv)
{
        return hb_mc_device_pod_kernel_enqueue(device, device->default_pod_id,
                                               grid_dim,
                                               tg_dim,
                                               name, argc, argv);
}




/**
 * Iterates over all tile groups inside device,
 * allocates those that fit in mesh and launches them.
 * API remains in this function until all tile groups
 * have successfully finished execution.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((weak))
int hb_mc_device_tile_groups_execute (hb_mc_device_t *device)
{
        return hb_mc_device_pod_kernels_execute(device, device->default_pod_id);
}
