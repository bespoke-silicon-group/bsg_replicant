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
#include <bsg_manycore_tile.h>
#include <bsg_manycore_memory_manager.h>
#include <bsg_manycore_elf.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_origin_eva_map.h>


#ifdef __cplusplus
#include <cstring>
#else
#include <string.h>
#endif







__attribute__((warn_unused_result))
static int hb_mc_device_mesh_init (hb_mc_device_t *device,
                                   hb_mc_dimension_t dim);

__attribute__((warn_unused_result))
static int hb_mc_device_mesh_exit (hb_mc_mesh_t *mesh); 

__attribute__((warn_unused_result))
static int hb_mc_device_tile_groups_init (hb_mc_device_t *device); 

__attribute__((warn_unused_result))
static int hb_mc_device_tile_groups_exit (hb_mc_device_t *device); 

__attribute__((warn_unused_result))
static int hb_mc_device_tiles_are_free (hb_mc_device_t *device,
                                        hb_mc_coordinate_t origin,
                                        hb_mc_dimension_t dim);

__attribute__((warn_unused_result))
static int hb_mc_tile_group_initialize_tiles (hb_mc_device_t *device,
                                              hb_mc_tile_group_t *tg,
                                              hb_mc_coordinate_t origin); 

__attribute__((warn_unused_result))
static int hb_mc_tile_group_allocate_tiles (hb_mc_device_t *device,
                                            hb_mc_tile_group_t *tg);

__attribute__((warn_unused_result))
static int hb_mc_tile_group_enqueue (hb_mc_device_t* device,
                                     grid_id_t grid_id,
                                     hb_mc_coordinate_t tg_id,
                                     hb_mc_dimension_t grid_dim,
                                     hb_mc_dimension_t dim,
                                     const char* name,
                                     uint32_t argc,
                                     const uint32_t *argv);

__attribute__((warn_unused_result))
static int hb_mc_tile_group_kernel_init (hb_mc_tile_group_t *tg, 
                                         const char* name, 
                                         uint32_t argc, 
                                         const uint32_t *argv); 

__attribute__((warn_unused_result))
static int hb_mc_tile_group_launch (hb_mc_device_t *device,
                                    hb_mc_tile_group_t *tg);

__attribute__((warn_unused_result))
static int hb_mc_tile_group_deallocate_tiles(hb_mc_device_t *device,
                                             hb_mc_tile_group_t *tg);

__attribute__((warn_unused_result))
static int hb_mc_tile_group_exit (hb_mc_tile_group_t *tg); 

__attribute__((warn_unused_result))
static int hb_mc_tile_group_kernel_exit (hb_mc_kernel_t *kernel); 

__attribute__((warn_unused_result))
static int hb_mc_device_program_load (hb_mc_device_t *device);

__attribute__((warn_unused_result))
static int hb_mc_device_manycore_exit (hb_mc_manycore_t *mc); 

__attribute__((warn_unused_result))
static int hb_mc_device_program_exit (hb_mc_program_t *program); 

__attribute__((warn_unused_result))
static int hb_mc_program_binary_copy (hb_mc_program_t *program,
                                      const unsigned char *bin_data,
                                      size_t bin_size); 

__attribute__((warn_unused_result))
static int hb_mc_program_allocator_init (const hb_mc_config_t *cfg,
                                         hb_mc_program_t *program,
                                         const char *name,
                                         hb_mc_allocator_id_t id);

__attribute__((warn_unused_result))
static int hb_mc_program_allocator_exit (hb_mc_allocator_t *allocator); 

__attribute__((warn_unused_result))
static int hb_mc_device_all_tile_groups_finished(hb_mc_device_t *device);

__attribute__((warn_unused_result))
static int hb_mc_device_wait_for_tile_group_finish_any(hb_mc_device_t *device);

__attribute__((warn_unused_result))
static hb_mc_epa_t hb_mc_tile_group_get_finish_signal_addr(hb_mc_tile_group_t *tg);  

__attribute__((warn_unused_result))
static hb_mc_idx_t hb_mc_get_tile_id (hb_mc_coordinate_t origin, hb_mc_dimension_t dim, hb_mc_coordinate_t coord); 

__attribute__((warn_unused_result))
static int hb_mc_device_tiles_freeze (hb_mc_device_t *device,
                                      const hb_mc_coordinate_t *tiles,
                                      uint32_t num_tiles); 

__attribute__((warn_unused_result))
static int hb_mc_device_tiles_unfreeze (hb_mc_device_t *device,
                                        const hb_mc_eva_map_t *map,
                                        const hb_mc_coordinate_t *tiles,
                                        uint32_t num_tiles); 

__attribute__((warn_unused_result))
static int hb_mc_tile_set_symbol_val (hb_mc_manycore_t *mc,
                                      const hb_mc_eva_map_t *map,
                                      const unsigned char* bin,
                                      size_t bin_size,
                                      const hb_mc_coordinate_t *coord,
                                      const char* symbol,
                                      const uint32_t *val);

__attribute__((warn_unused_result))
static int hb_mc_device_tiles_set_config_symbols (hb_mc_device_t *device,
                                                  const hb_mc_eva_map_t *map, 
                                                  hb_mc_coordinate_t origin,
                                                  hb_mc_coordinate_t tg_id,
                                                  hb_mc_dimension_t tg_dim, 
                                                  hb_mc_dimension_t grid_dim,
                                                  const hb_mc_coordinate_t *tiles,
                                                  uint32_t num_tiles);

__attribute__((warn_unused_result))
static int hb_mc_device_tiles_set_runtime_symbols (hb_mc_device_t *device,
                                                   const hb_mc_eva_map_t *map, 
                                                   uint32_t argc, 
                                                   hb_mc_eva_t args_eva,
                                                   hb_mc_npa_t finish_signal_npa, 
                                                   hb_mc_eva_t kernel_eva,      
                                                   const hb_mc_coordinate_t *tiles,
                                                   uint32_t num_tiles); 








/**
 * Takes in a hb_mc_device_t struct and initializes a mesh of tile in the Manycore device.
 * @param[in]  device        Pointer to device
 * @parma[in]  dim           X/Y dimensions of the tile pool (mesh) to be initialized
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
static int hb_mc_device_mesh_init (hb_mc_device_t *device, hb_mc_dimension_t dim){ 

        int error;

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device->mc);
        hb_mc_dimension_t device_dim = hb_mc_config_get_dimension_vcore(cfg);
        
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
                bsg_pr_err("%s: Mesh Y dimension (%d) larger than device Y dimension.\n",
                           __func__,
                           hb_mc_dimension_get_y(dim),
                           hb_mc_dimension_get_y(device_dim));
                return HB_MC_INVALID;
        }


        device->mesh = (hb_mc_mesh_t *) malloc (sizeof (hb_mc_mesh_t));
        if (device->mesh == NULL) { 
                bsg_pr_err("%s: failed to allocate space on host for hb_mc_mesh_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }
        
        device->mesh->dim = dim;
        device->mesh->origin = hb_mc_config_get_origin_vcore(cfg); 
        device->mesh->tiles = (hb_mc_tile_t *) malloc ( hb_mc_dimension_to_length(dim) * sizeof (hb_mc_tile_t));
        if (device->mesh->tiles == NULL) {
                bsg_pr_err("%s: failed to allocate space on host for hb_mc_tile_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }
        
        for (int x = hb_mc_coordinate_get_x(device->mesh->origin);
             x < hb_mc_coordinate_get_x(device->mesh->origin) + hb_mc_dimension_get_x(dim); x++){
                for (int y = hb_mc_coordinate_get_y(device->mesh->origin);
                     y < hb_mc_coordinate_get_y(device->mesh->origin) + hb_mc_dimension_get_y(dim); y++){
                        hb_mc_idx_t tile_id = hb_mc_get_tile_id (device->mesh->origin, dim, hb_mc_coordinate(x, y));    
                        device->mesh->tiles[tile_id].coord = hb_mc_coordinate(x, y);
                        device->mesh->tiles[tile_id].origin = device->mesh->origin;
                        device->mesh->tiles[tile_id].tile_group_id = hb_mc_coordinate(-1, -1); 
                        device->mesh->tiles[tile_id].status = HB_MC_TILE_STATUS_FREE;
                }
        }

        return HB_MC_SUCCESS;   
}




/**
 * Frees memroy and removes device's mesh (tile pool) object
 * @param[in]  mesh   Pointer to mesh struct
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
static int hb_mc_device_mesh_exit (hb_mc_mesh_t *mesh) { 
        int error;

        if (!mesh) { 
                bsg_pr_err("%s: calling exit on null mesh.\n", __func__); 
                return HB_MC_INVALID;
        }

        // Free tile list 
        const hb_mc_tile_t *tiles;
        tiles = mesh->tiles;
        if (!tiles) { 
                bsg_pr_err("%s: calling exit on mesh with null tile list.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((hb_mc_tile_t *) tiles); 
                mesh->tiles = NULL;
        }
        free(mesh);

        return HB_MC_SUCCESS;
}




/**
 * Enqueues and schedules a kernel to be run on device
 * Takes the grid size, tile group dimensions, kernel name, argc, argv* and the
 * finish signal address, calls hb_mc_tile_group_enqueue to initialize all tile groups for grid.
 * @param[in]  device        Pointer to device
 * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
 * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
 * @param[in]  name          Kernel name to be executed on tile groups in grid
 * @param[in]  argc          Number of input arguments to kernel
 * @param[in]  argv          List of input arguments to kernel
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_kernel_enqueue (hb_mc_device_t *device,
                               hb_mc_dimension_t grid_dim,
                               hb_mc_dimension_t tg_dim,
                               const char* name,
                               uint32_t argc,
                               const uint32_t *argv) {
        int error; 
        for (hb_mc_idx_t tg_id_x = 0; tg_id_x < hb_mc_dimension_get_x(grid_dim); tg_id_x ++) { 
                for (hb_mc_idx_t tg_id_y = 0; tg_id_y < hb_mc_dimension_get_y(grid_dim); tg_id_y ++) { 
                        hb_mc_coordinate_t tg_id = hb_mc_coordinate(tg_id_x, tg_id_y);
                        error = hb_mc_tile_group_enqueue(device, device->num_grids, tg_id, grid_dim, tg_dim, name, argc, argv); 
                        if (error != HB_MC_SUCCESS) { 
                                bsg_pr_err("%s: failed to initialize tile group (%d,%d) of grid %d.\n",
                                           __func__,
                                           tg_id_x, tg_id_y,
                                           device->num_grids);
                                return HB_MC_UNINITIALIZED;
                        }
                }
        }
        device->num_grids ++;
        return HB_MC_SUCCESS;
}





/**
 * Initializes the tile group structure of a device 
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_tile_groups_init (hb_mc_device_t *device) { 
        
        device->tile_group_capacity = 1;
        device->tile_groups = (hb_mc_tile_group_t *) malloc (device->tile_group_capacity * sizeof(hb_mc_tile_group_t));
        if (device->tile_groups == NULL) {
                bsg_pr_err("%s: failed to allocated space for list of tile groups.\n", __func__);
                return HB_MC_NOMEM;
        }
        memset (device->tile_groups, 0, device->tile_group_capacity * sizeof(hb_mc_tile_group_t));
        device->num_tile_groups = 0;
        return HB_MC_SUCCESS;
}




/**
 * Destructs the tile group structure of a device 
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_tile_groups_exit (hb_mc_device_t *device) { 
        int error;

        if (!device) { 
                bsg_pr_err("%s: calling exit on tile group list in null device.\n", __func__); 
                return HB_MC_INVALID;
        }


        if (!device->tile_groups) { 
                bsg_pr_err("%s: calling exit on null tile group list.\n", __func__); 
                return HB_MC_INVALID;
        }


        for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num++) { 
                error = hb_mc_tile_group_exit(&(device->tile_groups[tg_num])); 
                if ( error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to remove tile group struct.\n", __func__);
                        return error;
                }
        }


        // Free tile group list 
        const hb_mc_tile_group_t *tile_groups;
        tile_groups = device->tile_groups;
        if (!tile_groups) { 
                bsg_pr_err("%s: calling exit on device with null tile group list.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((hb_mc_tile_group_t *) tile_groups); 
                device->tile_groups = NULL;
        }

        return HB_MC_SUCCESS;
}





/**
 * Checks if a groups of tiles starting from a specific origin are all free or not.
 * @param[in]  device        Pointer to device
 * @param[in]  origin        Origin of the group of tiles to check for availability
 * @param[in]  dim           Dimension of the group of tiles in question
 * @return HB_MC_SUCCESS if all tiles in the group are free, otherwise an error code is returned. 
 */
static int hb_mc_device_tiles_are_free (hb_mc_device_t *device,
                                        hb_mc_coordinate_t origin,
                                        hb_mc_dimension_t dim) { 
        if (hb_mc_coordinate_get_x(origin) + hb_mc_dimension_get_x(dim) > 
            hb_mc_coordinate_get_x(device->mesh->origin) + hb_mc_dimension_get_x(device->mesh->dim)) { 
                bsg_pr_err ("%s: a %dx%d tile group starting from origin (%d,%d) \
                              does not fit in %dx%d device mesh. Check X dimension or origin.\n",
                            __func__, 
                            hb_mc_dimension_get_x(dim), hb_mc_dimension_get_y(dim),
                            hb_mc_coordinate_get_x(origin), hb_mc_coordinate_get_y(origin), 
                            hb_mc_dimension_get_x(device->mesh->dim), hb_mc_dimension_get_y(device->mesh->dim)); 
                return HB_MC_INVALID;
        }
                                
        if (hb_mc_coordinate_get_y(origin) + hb_mc_dimension_get_y(dim) >
            hb_mc_coordinate_get_y(device->mesh->origin) + hb_mc_dimension_get_y(device->mesh->dim)) { 
                bsg_pr_err ("%s: a %dx%d tile group starting from origin (%d,%d) \
                             does not fit in %dx%d device mesh. Check Y dimension or origin.\n",
                            __func__, 
                            hb_mc_dimension_get_x(dim), hb_mc_dimension_get_y(dim),
                            hb_mc_coordinate_get_x(origin), hb_mc_coordinate_get_y(origin), 
                            hb_mc_dimension_get_x(device->mesh->dim), hb_mc_dimension_get_y(device->mesh->dim)); 
                return HB_MC_INVALID;
        }


        hb_mc_coordinate_t tile_coord;
        hb_mc_idx_t tile_id;
                        
        // Iterate over a tg->dim.x * tg->dim.y square of tiles starting from the (org_x,org_y)
        // checking to see if all tiles are free so they can be allocated to tile group
        for (hb_mc_idx_t x = hb_mc_coordinate_get_x(origin);
             x < hb_mc_coordinate_get_x(origin) + hb_mc_dimension_get_x(dim); x++){
                for (hb_mc_idx_t y = hb_mc_coordinate_get_y(origin);
                     y < hb_mc_coordinate_get_y(origin) + hb_mc_dimension_get_y(dim); y++){
                        tile_id = hb_mc_get_tile_id (device->mesh->origin, device->mesh->dim, hb_mc_coordinate(x, y)); 
                        if (device->mesh->tiles[tile_id].status != HB_MC_TILE_STATUS_FREE) 
                                return HB_MC_FAIL;
                }
        }

        return HB_MC_SUCCESS;
}




/**
 * Takes in a device and tile group and an origin, initializes tile group
 * and sends packets to all tiles in tile group to set their symbols. 
 * @param[in]  device        Pointer to device
 * @param[in]  tg            Pointer to tile group
 * @param[in]  origin        Origin coordinates of tile group
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_tile_group_initialize_tiles (hb_mc_device_t *device,
                                              hb_mc_tile_group_t *tg,
                                              hb_mc_coordinate_t origin) { 

        int error;

        tg->origin = origin;

        error = hb_mc_origin_eva_map_init (tg->map, origin); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n",
                           __func__,
                           tg->grid_id,
                           hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
                return error;
        }



        // Define a list of tiles who's configuration symbols are to be set
        uint32_t num_tiles = hb_mc_dimension_to_length(tg->dim); 
        hb_mc_coordinate_t tile_list[num_tiles];
        
        hb_mc_idx_t tg_tile_id = 0;

        // Set tiles variables inside hb_mc_device_t struct
        // And prepare a list of tile coordinates who's config symbols are to be set 
        for (hb_mc_idx_t x = hb_mc_coordinate_get_x(origin);
             x < hb_mc_coordinate_get_x(origin) + hb_mc_dimension_get_x(tg->dim); x++){
                for (hb_mc_idx_t y = hb_mc_coordinate_get_y(origin);
                     y < hb_mc_coordinate_get_y(origin) + hb_mc_dimension_get_y(tg->dim); y++){
                        hb_mc_idx_t device_tile_id = hb_mc_get_tile_id (device->mesh->origin, device->mesh->dim, hb_mc_coordinate(x, y));

                        device->mesh->tiles[device_tile_id].origin = origin;
                        device->mesh->tiles[device_tile_id].tile_group_id = tg->id;
                        device->mesh->tiles[device_tile_id].status = HB_MC_TILE_STATUS_BUSY;


                        tile_list[tg_tile_id] = hb_mc_coordinate (x, y); 
                        tg_tile_id ++;
                }
        }




        // Set the configuration symbols of all tiles inside tile group
        error = hb_mc_device_tiles_set_config_symbols(device,
                                                      tg->map,
                                                      tg->origin,
                                                      tg->id,
                                                      tg->dim,
                                                      tg->grid_dim,
                                                      tile_list,
                                                      num_tiles);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to set grid %d tile group (%d,%d) tiles configuration symbols.\n", 
                           __func__,
                           tg->grid_id,
                           hb_mc_coordinate_get_x (tg->id),
                           hb_mc_coordinate_get_y (tg->id));
                return error;
        }



        tg->status = HB_MC_TILE_GROUP_STATUS_ALLOCATED;


        bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) allocated at origin (%d,%d).\n", 
                   __func__,
                   tg-> grid_id,
                   hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
                   hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));     


        return HB_MC_SUCCESS;
}






/**
 * Searches for a free tile group inside the device mesh and allocoates it,
 * and sets the dimensions, origin, and id of tile group.
 * @param[in]  device        Pointer to device
 * @param[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_tile_group_allocate_tiles (hb_mc_device_t *device,
                                            hb_mc_tile_group_t *tg){
        int error;
        if (hb_mc_dimension_get_x(tg->dim) > hb_mc_dimension_get_x(device->mesh->dim)){
                bsg_pr_err("%s: tile group X dimension (%d) larger than mesh X dimension (%d).\n",
                           __func__,
                           hb_mc_dimension_get_x(tg->dim),
                           hb_mc_dimension_get_x(device->mesh->dim));
                return HB_MC_INVALID;
        }
        if (hb_mc_dimension_get_y(tg->dim) > hb_mc_dimension_get_y(device->mesh->dim)){
                bsg_pr_err("%s: tile group Y dimension (%d) larger than mesh Y dimension (%d).\n",
                           __func__,
                           hb_mc_dimension_get_y(tg->dim),
                           hb_mc_dimension_get_y(device->mesh->dim));
                return HB_MC_INVALID;
        }

        hb_mc_coordinate_t tile_coord;
        hb_mc_idx_t tile_id; 

        // Iterate over the entire mesh as tile (org_y, org_x) being the origin of the new tile group to allcoate 
        for (hb_mc_idx_t org_y = hb_mc_coordinate_get_y(device->mesh->origin);
             org_y <= (hb_mc_coordinate_get_y(device->mesh->origin)
                       + hb_mc_dimension_get_y(device->mesh->dim)
                       - hb_mc_dimension_get_y(tg->dim)); org_y++){
                for (hb_mc_idx_t org_x = hb_mc_coordinate_get_x(device->mesh->origin);
                     org_x <= (hb_mc_coordinate_get_x(device->mesh->origin)
                               + hb_mc_dimension_get_x(device->mesh->dim)
                               - hb_mc_dimension_get_x(tg->dim)); org_x++){


                        // Search if a tg->dim.x * tg->dim.y group of tiles starting from (org_x,org_y) are all free
                        if (hb_mc_device_tiles_are_free(device, hb_mc_coordinate(org_x, org_y), tg->dim) == HB_MC_SUCCESS) { 

                                // Found a free group of tiles at origin (org_x, org_y), now initialize
                                // all these tiles by sending packets and claiming them for this tile group
                                error = hb_mc_tile_group_initialize_tiles (device, tg, hb_mc_coordinate(org_x, org_y));
                                if (error != HB_MC_SUCCESS) { 
                                        bsg_pr_err("%s: failed to initialize mesh tiles for grid %d tile group (%d,%d).\n",
                                                   __func__,
                                                   tg->grid_id,
                                                   hb_mc_coordinate_get_x(tg->id),
                                                   hb_mc_coordinate_get_y(tg->id)); 
                                        return error;
                                }
                                return HB_MC_SUCCESS;
                        }
                }
        }
        return HB_MC_NOTFOUND;
}




/**
 * Takes the kernel name, argc, argv* and the finish signal address, and initializes a kernel and passes it to tilegroup.
 * @param[in]  device        Pointer to device
 * @parma[in]  grid_id       Id of grid to which the tile group belongs
 * @param[in]  tg_id         Id of tile group
 * @param[in]  grid_dim      X/Y dimensions of grid to which tile group belongs
 * @parma[in]  dim           X/Y dimensions of tie pool (mesh)
 * @param[in]  name          Kernel name that is to be executed on tile group
 * @param[in]  argc          Number of input arguments to the kernel
 * @param[in]  argv          List of input arguments to the kernel
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_tile_group_enqueue (hb_mc_device_t* device,
                                     grid_id_t grid_id,
                                     hb_mc_coordinate_t tg_id,
                                     hb_mc_dimension_t grid_dim,
                                     hb_mc_dimension_t dim,
                                     const char* name,
                                     uint32_t argc,
                                     const uint32_t *argv) {

        if (device->num_tile_groups == device->tile_group_capacity) { 
                device->tile_group_capacity *= 2;
                device->tile_groups = (hb_mc_tile_group_t *) realloc (device->tile_groups, device->tile_group_capacity * sizeof(hb_mc_tile_group_t));
                if (device->tile_groups == NULL) {
                        bsg_pr_err("%s: failed to allocate space for hb_mc_tile_group_t structs.\n", __func__);
                        return HB_MC_NOMEM;
                }
        }
        

        hb_mc_tile_group_t* tg = &device->tile_groups[device->num_tile_groups];
        tg->dim = dim;
        tg->origin = device->mesh->origin;
        tg->id = tg_id;
        tg->grid_id = grid_id;
        tg->grid_dim = grid_dim;
        tg->status = HB_MC_TILE_GROUP_STATUS_INITIALIZED;

        tg->map = (hb_mc_eva_map_t *) malloc (sizeof(hb_mc_eva_map_t)); 
        if (tg->map == NULL) { 
                bsg_pr_err ("%s: failed to allocate space for tile group hb_mc_eva_map_t struct map.\n", __func__);
                return HB_MC_NOMEM;
        }
        int error = hb_mc_origin_eva_map_init (tg->map, tg->origin); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n",
                           __func__,
                           tg->grid_id,
                           tg->id.x, tg->id.y); 
                return HB_MC_UNINITIALIZED;
        }
        

        error = hb_mc_tile_group_kernel_init (tg, name, argc, argv); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize tile group's kernel\n", __func__);
                return error;
        }

                        
        device->num_tile_groups += 1;
        
        bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) initialized.\n", 
                   __func__,
                   tg->grid_id,
                   hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id)) ;

        return HB_MC_SUCCESS;
}




/**
 * Takes in a kernel name, argument count and list of arguments 
 * and initializes a hb_mc_kernel_t struct for the tile group.
 * @param[in]  tg            Pointer to tile group. 
 * @param[in]  name          Kernel name that is to be executed on tile group
 * @param[in]  argc          Number of input arguments to the kernel
 * @param[in]  argv          List of input arguments to the kernel
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
__attribute__((warn_unused_result))
static int hb_mc_tile_group_kernel_init (hb_mc_tile_group_t *tg, 
                                         const char* name, 
                                         uint32_t argc, 
                                         const uint32_t *argv) {
        tg->kernel = (hb_mc_kernel_t *) malloc (sizeof(hb_mc_kernel_t));
        if (tg->kernel == NULL) { 
                bsg_pr_err("%s: failed to allocated space for hb_mc_kernel_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }
        tg->kernel->name = strdup (name); 
        if (tg->kernel->name == NULL) { 
                bsg_pr_err("%s: failed to allocate space on device for tile group's kernel's name.\n", __func__);
                return HB_MC_NOMEM;
        }
        tg->kernel->argc = argc;
        uint32_t *cpy = (uint32_t *) malloc (tg->kernel->argc * sizeof(uint32_t)); 
        if (cpy == NULL) { 
                bsg_pr_err("%s: failed to allocate space on devcie for kernel's argument list.\n", __func__); 
                return HB_MC_NOMEM;
        }
        memcpy (cpy, argv, argc * sizeof(uint32_t));    
        tg->kernel->argv = (const uint32_t *) cpy;
        tg->kernel->finish_signal_addr = hb_mc_tile_group_get_finish_signal_addr(tg); 

        return HB_MC_SUCCESS;
}







/**
 * Launches a tile group by sending packets to each tile in the
 * tile group setting the argc, argv, finish_addr and kernel pointer.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if tile group is launched successfully, otherwise an error code is returned.
 */
static int hb_mc_tile_group_launch (hb_mc_device_t *device,
                                    hb_mc_tile_group_t *tg) {

        int error;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config (device->mc); 

        hb_mc_eva_t args_eva;

        // allocate device memory for arguments
        error = hb_mc_device_malloc (device, (tg->kernel->argc) * sizeof(uint32_t), &args_eva);
        if (error != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to allocate space on device for grid %d tile group (%d,%d) arguments.\n",
                           __func__,
                           tg->grid_id,
                           hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
                return HB_MC_NOMEM;
        }

        // transfer the arguments to dram
        error = hb_mc_device_memcpy(    device, reinterpret_cast<void *>(args_eva),
                                        (void *) &(tg->kernel->argv[0]),
                                        (tg->kernel->argc) * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
        if (error != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to copy grid %d tile group (%d,%d) arguments to device.\n",
                           __func__,
                           tg->grid_id, hb_mc_coordinate_get_x(tg->id),
                           hb_mc_coordinate_get_y(tg->id)); 
                return error;
        }
        
        hb_mc_eva_t kernel_eva; 
        error = hb_mc_loader_symbol_to_eva (device->program->bin, device->program->bin_size, tg->kernel->name, &kernel_eva); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: invalid kernel name %s for grid %d tile group (%d,%d).\n",
                           __func__,
                           tg->kernel->name,
                           tg->grid_id,
                           hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
                return error;
        }       

        hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 
        hb_mc_npa_t finish_signal_npa = hb_mc_npa(host_coordinate, tg->kernel->finish_signal_addr); 


        // Create a list of tile coordinates for tiles inside tile group 
        uint32_t num_tiles = hb_mc_dimension_to_length(tg->dim); 
        hb_mc_coordinate_t tile_list[num_tiles];

        int tg_tile_id = 0;
        for (   hb_mc_idx_t y = hb_mc_coordinate_get_y(tg->origin);
                y < hb_mc_coordinate_get_y(tg->origin) + hb_mc_dimension_get_y(tg->dim); y++){
                for (   hb_mc_idx_t x = hb_mc_coordinate_get_x(tg->origin);
                        x < hb_mc_coordinate_get_x(tg->origin) + hb_mc_dimension_get_x(tg->dim); x++){
                        tile_list[tg_tile_id] = hb_mc_coordinate (x, y); 
                        tg_tile_id ++;
                }
        }
        

        // Set the runtime symbols of all tiles inside tile group
        error = hb_mc_device_tiles_set_runtime_symbols(device,
                                                       tg->map,
                                                       tg->kernel->argc,
                                                       args_eva,
                                                       finish_signal_npa, 
                                                       kernel_eva,
                                                       tile_list,
                                                       num_tiles);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to set grid %d tile group (%d,%d) tiles runtime symbols.\n", 
                           __func__,
                           tg->grid_id,
                           hb_mc_coordinate_get_x (tg->id),
                           hb_mc_coordinate_get_y (tg->id));
                return error;
        }




        tg->status=HB_MC_TILE_GROUP_STATUS_LAUNCHED;
        bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) launched at origin (%d,%d).\n",
                   __func__,
                   tg->grid_id,
                   hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
                   hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));

        return HB_MC_SUCCESS;
}




/**
 * De-allocates all tiles in tile group, and resets their tile-group id and origin in the device book keeping.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_tile_group_deallocate_tiles(hb_mc_device_t *device,
                                             hb_mc_tile_group_t *tg) {
        int error;
        hb_mc_idx_t tile_id; 
        for (   int x = hb_mc_coordinate_get_x(tg->origin);
                x < hb_mc_coordinate_get_x(tg->origin) + hb_mc_dimension_get_x(tg->dim); x++){
                for (   int y = hb_mc_coordinate_get_y(tg->origin);
                        y < hb_mc_coordinate_get_y(tg->origin) + hb_mc_dimension_get_y(tg->dim); y++){
                        tile_id = hb_mc_get_tile_id (device->mesh->origin, device->mesh->dim, hb_mc_coordinate (x, y));  
                        
                        device->mesh->tiles[tile_id].origin = device->mesh->origin;
                        device->mesh->tiles[tile_id].tile_group_id = hb_mc_coordinate( 0, 0);
                        device->mesh->tiles[tile_id].status = HB_MC_TILE_STATUS_FREE;
                }
        }
        bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) de-allocated at origin (%d,%d).\n",
                   __func__,
                   tg->grid_id,
                   hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
                   hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));
        
        tg->status = HB_MC_TILE_GROUP_STATUS_FINISHED;

        return HB_MC_SUCCESS;
}





/**
 * Frees memroy and removes tile group object
 * @param[in]  tg        Pointer to tile group
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
__attribute__((warn_unused_result))
static int hb_mc_tile_group_exit (hb_mc_tile_group_t *tg) {
        int error;

        if (!tg) { 
                bsg_pr_err("%s: calling exit on null tile group.\n", __func__); 
                return HB_MC_INVALID;
        }

        error = hb_mc_tile_group_kernel_exit (tg->kernel); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err ("%s: failed to remove tile group's kernel object.\n", __func__);
                return error;
        }

        error = hb_mc_origin_eva_map_exit (tg->map);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err ("%s: failed to delete tile group's map object.\n", __func__);
                return error;
        }
        tg = NULL;      

        return HB_MC_SUCCESS;
}





/**
 * Frees memroy and removes kernel object
 * @param[in]  kernel    Pointer kernel
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
__attribute__((warn_unused_result))
static int hb_mc_tile_group_kernel_exit (hb_mc_kernel_t *kernel) {
        if (!kernel) { 
                bsg_pr_err("%s: calling exit on null kernel object.\n", __func__);
                return HB_MC_INVALID;
        }


        // Free name
        const char* name;
        name = kernel->name;
        if (!name) { 
                bsg_pr_err("%s: calling exit on kernel with null name.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((void *) name); 
                kernel->name = NULL;
        }


        // Free argv
        const uint32_t *argv;
        argv = kernel->argv;
        if (!argv) { 
                bsg_pr_err("%s: calling exit on kernel with null argument list.\n", __func__);
                return HB_MC_INVALID;
        } else { 
                free ((void*)argv); 
                kernel->argv = NULL;
        }
        free(kernel);
        
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
                       hb_mc_manycore_id_t id){

        return hb_mc_device_init_custom_dimensions(device, name, id, {0, 0});
}


/**
 * Initializes the manycore struct, and a mesh structure with custom 
 * dimensions inside device struct with list of tiles and their coordinates 
 * @param[in]  device        Pointer to device
 * @param[in]  name          Device name
 * @param[in]  id            Device id
 * @param[in]  dim           Tile pool (mesh) dimensions. If (0,0) this method will initialize the whole array.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned. 
 */
int hb_mc_device_init_custom_dimensions (hb_mc_device_t *device,
                                         const char *name,
                                         hb_mc_manycore_id_t id,
                                         hb_mc_dimension_t dim) {

        device->mc = (hb_mc_manycore_t*) malloc (sizeof (hb_mc_manycore_t));
        if (device->mc == NULL) { 
                bsg_pr_err("%s: failed to allocate space on host for hb_mc_manycore_t.\n", __func__);
                return HB_MC_NOMEM;
        }
        *(device->mc) = {0};
        
        int error = hb_mc_manycore_init(device->mc, name, id); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize manycore.\n", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // If the input dimensions are (0,0) this will initialize the whole array.
        if(!dim.x && !dim.y){
                dim = hb_mc_config_get_dimension_vcore(hb_mc_manycore_get_config(device->mc));
        }
        
        error = hb_mc_device_mesh_init(device, dim);
        if (error != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to initialize mesh.\n", __func__);
                return HB_MC_UNINITIALIZED;
        }

        error = hb_mc_device_tile_groups_init (device); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize device's tile group structure.\n", __func__);
                return error; 
        }

        device->num_grids = 0;

        return HB_MC_SUCCESS;
}




/**
 * Loads the binary in a device's hb_mc_program_t struct
 * onto all tiles in device's hb_mc_mesh_t struct. 
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_program_load (hb_mc_device_t *device) { 
        int error; 

        // Create list of tile coordinates 
        uint32_t num_tiles = hb_mc_dimension_to_length (device->mesh->dim);
        hb_mc_coordinate_t tile_list[num_tiles];
        for (int tile_id = 0; tile_id < num_tiles; tile_id ++) {
                tile_list[tile_id] = device->mesh->tiles[tile_id].coord;
        }

        // Freeze tiles
        error = hb_mc_device_tiles_freeze(device, tile_list, num_tiles);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to freeze device tiles.\n", __func__); 
                return error;
        }


        // Load binary into all tiles 
        error = hb_mc_loader_load (device->program->bin,
                                   device->program->bin_size,
                                   device->mc,
                                   &default_map,
                                   &tile_list[0],
                                   num_tiles); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err ("%s: failed to load binary into tiles.\n", __func__); 
                return error;
        }       


        // Set all tiles configuration symbols 
        hb_mc_coordinate_t tg_id = hb_mc_coordinate (0, 0);
        hb_mc_coordinate_t tg_dim = hb_mc_coordinate (1, 1); 
        hb_mc_coordinate_t grid_dim = hb_mc_coordinate (1, 1); 

        error = hb_mc_device_tiles_set_config_symbols(device,
                                                      &default_map,
                                                      device->mesh->origin,
                                                      tg_id,
                                                      tg_dim, 
                                                      grid_dim,
                                                      tile_list,
                                                      num_tiles);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to set tiles configuration symbols.\n", __func__);
                return error;
        }       
                                                                        


        // Unfreeze all tiles 
        error = hb_mc_device_tiles_unfreeze(device, &default_map, tile_list, num_tiles); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to unfreeze device tiles.\n", __func__); 
                return error;
        }       

        return HB_MC_SUCCESS;
}





/**
 * Takes in a buffer containing binary and its size,
 * freezes tiles, loads program binary into all tiles and into dram,
 * and sets the symbols and registers for each tile.
 * @param[in]  device        Pointer to device
 * @parma[in]  bin_name      Name of binary elf file
 * @param[in]  bin_data      Buffer containing binary
 * @param[in]  bin_size      Size of binary to be loaded onto device
 * @param[in]  id            Id of program's meomry allocator
 * @param[in]  alloc_name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_program_init_binary (hb_mc_device_t *device, 
                                      const char *bin_name,
                                      const unsigned char* bin_data, 
                                      size_t bin_size, 
                                      const char* alloc_name, 
                                      hb_mc_allocator_id_t id) { 
        int error;
        
        device->program = (hb_mc_program_t *) malloc (sizeof (hb_mc_program_t));
        if (device->program == NULL) { 
                bsg_pr_err("%s: failed to allocate space on host for device hb_mc_program_t struct.\n", __func__);
                return HB_MC_NOMEM;
        }

        device->program->bin_name = strdup (bin_name);
        if (!device->program->bin_name) { 
                bsg_pr_err("%s: failed to copy binary name into program struct.\n", __func__); 
                return HB_MC_NOMEM;
        }


        // Copy binary into device's program struct and set it's size in characters
        error = hb_mc_program_binary_copy (device->program,
                                           bin_data,
                                           bin_size);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to copy binary into program struct.\n", __func__); 
                return error;
        }


        // Initialize program's memory allocator
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device->mc); 
        error = hb_mc_program_allocator_init (cfg, device->program, alloc_name, id); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize memory allocator for program %s.\n", __func__, device->program->bin_name); 
                return HB_MC_UNINITIALIZED;
        }

        // Load binary onto all tiles
        error = hb_mc_device_program_load (device); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to load program binary onto device tiles.\n", __func__);
                return error;
        }

        return HB_MC_SUCCESS;
}





/**
 * Takes in a binary name, loads the binary from file onto a buffer,
 * freezes tiles, loads program binary into all tiles and into dram,
 * and sets the symbols and registers for each tile.
 * @param[in]  device        Pointer to device
 * @parma[in]  bin_name      Name of binary elf file
 * @param[in]  id            Id of program's meomry allocator
 * @param[in]  alloc_name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_program_init (hb_mc_device_t *device,
                               const char *bin_name,
                               const char *alloc_name,
                               hb_mc_allocator_id_t id) {
        int error; 
        
        unsigned char* bin_data;
        size_t bin_size;

        error = hb_mc_loader_read_program_file (bin_name, &bin_data, &bin_size);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err ("%s: failed to read binary file.\n", __func__); 
                return error;
        }


        error = hb_mc_device_program_init_binary (      device, bin_name, 
                                                        bin_data, bin_size, 
                                                        alloc_name, id); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to initialize device with program binary.\n", __func__);
                return error;
        }

        return HB_MC_SUCCESS;
}





/**
 * Frees memroy and removes device's manycore object
 * @param[in]  mc        Pointer to manycore struct
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
static int hb_mc_device_manycore_exit (hb_mc_manycore_t *mc) { 
        int error;

        if (!mc) { 
                bsg_pr_err("%s: calling exit on null manycore.\n", __func__); 
                return HB_MC_INVALID;
        }

        error = hb_mc_manycore_exit(mc); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to exit manycore.\n", __func__);
                return error;
        }
        free(mc);

        return HB_MC_SUCCESS;
}




/**
 * Frees memroy and removes program object
 * @param[in]  program   Pointer to program struct
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
static int hb_mc_device_program_exit (hb_mc_program_t *program) { 
        int error;

        if (!program) { 
                bsg_pr_err("%s: calling exit on null program.\n", __func__); 
                return HB_MC_INVALID;
        }


        const char* bin_name;
        const unsigned char* bin;

        // Free name
        bin_name = program->bin_name;
        if (!bin_name) { 
                bsg_pr_err("%s: calling exit on program with null binary name.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((void *) bin_name); 
                program->bin_name = NULL;
        }


        // Free binary
        bin = program->bin;
        if (!bin) { 
                bsg_pr_err("%s: calling exit on program with null binary.\n", __func__);
                return HB_MC_INVALID;
        } else {
                free ((void *) bin); 
                program->bin = NULL;
        }

        // Free allocator
        error = hb_mc_program_allocator_exit (program->allocator); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to destruct program's allocator struct.\n", __func__); 
                return error;
        }
        free(program);

        return HB_MC_SUCCESS;
}




/**
 * Copies the binary into the hb_mc_program_t struct's binary section
 * And sets the program's binary size in characters 
 * @param[in]  program       Pointer to program
 * @param[in]  bin_data      Buffer containing the binary
 * @param[in]  bin_size      Size of binary in characters
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_program_binary_copy (hb_mc_program_t *program,
                                      const unsigned char *bin_data,
                                      size_t bin_size) { 

        if (!bin_data) {
                bsg_pr_err("%s: binary is null.\n", __func__);
                return HB_MC_INVALID;
        }

        unsigned char* copy = (unsigned char*) malloc (bin_size);
        if (!copy){ 
                bsg_pr_err("%s: failed to allocated space on device for program binary.\n", __func__);
                return HB_MC_NOMEM;
        }

        memcpy((unsigned char*) copy, (const unsigned char*) bin_data, bin_size);
        
        program->bin = copy; 
        program->bin_size = bin_size;

        return HB_MC_SUCCESS;   
}




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

        uint32_t alignment = 32;
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
        int error;

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





        

/**
 * Checks to see if all tile groups in a device are finished.
 * @param[in]  device        Pointer to device
 * returns HB_MC_SUCCESS if all tile groups are finished, and HB_MC_FAIL otherwise.
 */
static int hb_mc_device_all_tile_groups_finished(hb_mc_device_t *device) {
        
        hb_mc_tile_group_t *tg = device->tile_groups; 
        for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) {
                if (tg->status != HB_MC_TILE_GROUP_STATUS_FINISHED )
                        return HB_MC_FAIL; 
        }

        return HB_MC_SUCCESS;
}




/**
 * Waits for a tile group to send a finish packet to device.
 * @param[in]  device        Pointer to device
 * return HB_MC_SUCCESS after a tile group is finished, gets stuck in infinite loop if no tile group finishes.
 */
static int hb_mc_device_wait_for_tile_group_finish_any(hb_mc_device_t *device) {
        int error; 

        int tile_group_finished = 0;
        hb_mc_request_packet_t recv, finish;
        hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 

        while (!tile_group_finished) {

                error = hb_mc_manycore_request_rx (device->mc, &recv, -1); 
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to read fifo for finish packet from device.\n", __func__);
                        return error;
                }

                /* Check all tile groups to see if the received packet is the finish packet from one of them */
                hb_mc_tile_group_t *tg = device->tile_groups;
                for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) {
                        if (tg->status == HB_MC_TILE_GROUP_STATUS_LAUNCHED) {

                                hb_mc_request_packet_set_x_dst(&finish, hb_mc_coordinate_get_x(host_coordinate));
                                hb_mc_request_packet_set_y_dst(&finish, hb_mc_coordinate_get_y(host_coordinate));
                                hb_mc_request_packet_set_x_src(&finish, hb_mc_coordinate_get_x(tg->origin));
                                hb_mc_request_packet_set_y_src(&finish, hb_mc_coordinate_get_y(tg->origin));
                                hb_mc_request_packet_set_data(&finish, HB_MC_CUDA_FINISH_SIGNAL_VAL);
                                hb_mc_request_packet_set_mask(&finish, HB_MC_PACKET_REQUEST_MASK_WORD);
                                hb_mc_request_packet_set_op(&finish, HB_MC_PACKET_OP_REMOTE_STORE);
                                hb_mc_request_packet_set_addr(&finish, tg->kernel->finish_signal_addr >> 2);

                                if (hb_mc_request_packet_equals(&recv, &finish) == HB_MC_SUCCESS) {
                
                                        bsg_pr_dbg("%s: Finish packet received for grid %d tile group (%d,%d): \
                                                    src (%d,%d), dst (%d,%d), addr: 0x%08" PRIx32 ", data: %d.\n", 
                                                   __func__, 
                                                   tg->grid_id, 
                                                   hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id), 
                                                   recv.x_src, recv.y_src, 
                                                   recv.x_dst, recv.y_dst, 
                                                   recv.addr, recv.data);

                                        error = hb_mc_tile_group_deallocate_tiles(device, tg);
                                        if (error != HB_MC_SUCCESS) { 
                                                bsg_pr_err("%s: failed to deallocate grid %d tile group (%d,%d).\n",
                                                           __func__,
                                                           tg->grid_id, hb_mc_coordinate_get_x(tg->id),
                                                           hb_mc_coordinate_get_y(tg->id));
                                                return error;
                                        }
                                        tile_group_finished = 1; 
                                        break;
                                }                               
                        }
                }       
        }

        return HB_MC_SUCCESS;
}




/**
 * Iterates over all tile groups inside device, allocates those that fit in mesh and launches them. 
 * API remains in this function until all tile groups have successfully finished execution.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_tile_groups_execute (hb_mc_device_t *device) {

        int error ;
        /* loop untill all tile groups have been allocated, launched and finished. */
        while(hb_mc_device_all_tile_groups_finished(device) != HB_MC_SUCCESS) {
                /* loop over all tile groups and try to launch as many as possible */
                hb_mc_tile_group_t *tg = device->tile_groups;
                for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) { 
                        if (tg->status == HB_MC_TILE_GROUP_STATUS_INITIALIZED) {
                                error = hb_mc_tile_group_allocate_tiles(device, tg) ;
                                if (error == HB_MC_SUCCESS) {
                                        error = hb_mc_tile_group_launch(device, tg);
                                        if (error != HB_MC_SUCCESS) {
                                                bsg_pr_err("%s: failed to launch tile group %d.\n", __func__, tg_num);
                                                return error;
                                        }
                                }
                        }
                }

                /* wait for a tile group to finish */
                error = hb_mc_device_wait_for_tile_group_finish_any(device);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: tile group not finished, something went wrong.\n", __func__); 
                        return error;
                }

        }

        return HB_MC_SUCCESS;
}




/**
 * Deletes memory manager, device and manycore struct, and freezes all tiles in device.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_finish (hb_mc_device_t *device) {

        int error;


        // Create list of tile coordinates 
        uint32_t num_tiles = hb_mc_dimension_to_length (device->mesh->dim);
        hb_mc_coordinate_t tile_list[num_tiles];
        for (int tile_id = 0; tile_id < num_tiles; tile_id ++) {
                tile_list[tile_id] = device->mesh->tiles[tile_id].coord;
        }


        error = hb_mc_device_tiles_freeze(device, tile_list, num_tiles); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to freeze device tiles.\n", __func__); 
                return error;
        }
        

        error = hb_mc_device_manycore_exit (device->mc); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to destruct device's manycore struct.\n", __func__);
                return error;
        }


        error = hb_mc_device_program_exit (device->program); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to destruct device's program struct.\n", __func__);
                return error;
        }


        error = hb_mc_device_mesh_exit (device->mesh); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to destruct device's mesh struct.\n", __func__);
                return error;
        }

        
        error = hb_mc_device_tile_groups_exit(device); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to destruct device's tile group list struct.\n", __func__);
                return error;
        }

        return HB_MC_SUCCESS;
}




/**
 * Allocates memory on device DRAM
 * hb_mc_device_program_init() or hb_mc_device_program_init_binary() should
 * have been called before calling this function to set up a memory allocator
 * @param[in]  device        Pointer to device
 * @parma[in]  size          Size of requested memory
 * @param[out] eva           Eva address of the allocated memory
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_malloc (hb_mc_device_t *device, uint32_t size, hb_mc_eva_t *eva) {
        *eva = 0;

        if (!device->program->allocator->memory_manager) {
                bsg_pr_err("%s: Memory manager not initialized.\n", __func__);
                return HB_MC_FAIL; 
        }

        awsbwhal::MemoryManager * mem_manager = (awsbwhal::MemoryManager *) device->program->allocator->memory_manager; 
        hb_mc_eva_t result = mem_manager->alloc(size);
        if (result == awsbwhal::MemoryManager::mNull) {
                bsg_pr_err("%s: failed to allocated memory.\n", __func__);      
                return HB_MC_NOMEM; 
        }
        *eva = result;
        return HB_MC_SUCCESS;
}




/**
 * Frees memory on device DRAM
 * hb_mc_device_program_init() or hb_mc_device_program_init_binary() should
 * have been called before calling this function to set up a memory allocator
 * @param[in]  device        Pointer to device
 * @param[out] eva           Eva address of the memory to be freed
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_free (hb_mc_device_t *device, eva_t eva) {


        if (!device->program->allocator->memory_manager) {
                bsg_pr_err("%s: memory manager not initialized.\n", __func__);
                return HB_MC_UNINITIALIZED; 
        }

        awsbwhal::MemoryManager * mem_manager = (awsbwhal::MemoryManager *) device->program->allocator->memory_manager; 
        mem_manager->free(eva);
        return HB_MC_SUCCESS;
}





/**
 * Copies a buffer from src on the host/device DRAM to dst on device DRAM/host.
 * @param[in]  device        Pointer to device
 * @parma[in]  src           EVA address of source to be copied from
 * @parma[in]  dst           EVA address of destination to be copied into
 * @param[in]  name          EVA address of dst
 * @param[in]  count         Size of buffer (number of words) to be copied
 * @param[in]  hb_mc_memcpy_kind         Direction of copy (HB_MC_MEMCPY_TO_DEVICE / HB_MC_MEMCPY_TO_HOST)
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_memcpy (hb_mc_device_t *device,
                         void *dst,
                         const void *src,
                         uint32_t count,
                         enum hb_mc_memcpy_kind kind) {

        int error;
        
        hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 
        size_t sz = count / sizeof(uint8_t); 

        if (kind == HB_MC_MEMCPY_TO_DEVICE) {
                hb_mc_eva_t dst_eva = (hb_mc_eva_t) reinterpret_cast<uintptr_t>(dst);

                error =  hb_mc_manycore_eva_write(device->mc, &default_map, &host_coordinate, &dst_eva, src, sz);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to copy memory to device.\n", __func__);
                        return error;
                }
        }
        
        else if (kind == HB_MC_MEMCPY_TO_HOST) { 
                hb_mc_eva_t src_eva = (hb_mc_eva_t) reinterpret_cast<uintptr_t>(src);

                error = hb_mc_manycore_eva_read(device->mc, &default_map, &host_coordinate, &src_eva, dst, sz); 
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to copy memory from device.\n", __func__);
                        return error;
                }

        }

        else {
                bsg_pr_err("%s: invalid copy type. Copy type can be one of \
                            HB_MC_MEMCPY_TO_DEVICE or HB_MC_MEMCPY_TO_HOST.\n", __func__);
                return HB_MC_INVALID; 
        }

        return HB_MC_SUCCESS;
}




/**
 * Sets memory to a give value starting from an address in device's DRAM.
 * @param[in]  device        Pointer to device
 * @parma[in]  eva           EVA address of destination 
 * @param[in]  val           Value to be written out
 * @param[in]  sz            The number of bytes to write into device DRAM
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_memset (hb_mc_device_t *device,
                         const hb_mc_eva_t *eva,
                         uint8_t val,
                         size_t sz) {

        int error;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config (device->mc); 
        hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 

        error = hb_mc_manycore_eva_memset (device->mc, 
                                           &default_map, 
                                           &host_coordinate, 
                                           eva,
                                           val,
                                           sz);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to set memory.\n", __func__);
                return error;
        }

        return HB_MC_SUCCESS;
}




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
 * Sends packets to all tiles in the list to freeze them 
 * @param[in]  device        Pointer to device
 * @param[in]  tiles         List of tile coordinates to freeze
 * @param[in]  num_tiles     Number of tiles in the list
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_tiles_freeze (hb_mc_device_t *device,
                                      const hb_mc_coordinate_t *tiles,
                                      uint32_t num_tiles) { 
        int error;
        for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) { 
                error = hb_mc_tile_freeze(device->mc, &tiles[tile_id]); 
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to freeze tile (%d,%d).\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }
        }
        return HB_MC_SUCCESS;
}





/**
 * Sends packets to all tiles in the list to set their kernel pointer to 1 and unfreeze them  
 * @param[in]  device        Pointer to device
 * @param[in]  map           EVA to NPA mapping for the tiles 
 * @param[in]  tiles         List of tile coordinates to unfreeze
 * @param[in]  num_tiles     Number of tiles in the list
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_tiles_unfreeze (hb_mc_device_t *device,
                                        const hb_mc_eva_map_t *map,
                                        const hb_mc_coordinate_t *tiles,
                                        uint32_t num_tiles) { 
        int error;
        hb_mc_eva_t kernel_eva = HB_MC_CUDA_KERNEL_NOT_LOADED_VAL;
        for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) {


                // Set the tile's cuda_kernel_ptr_eva symbol to HB_MC_CUDA_KERNEL_NOT_LOADED_VAL
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_kernel_ptr",
                                                  &kernel_eva);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_kernel_ptr symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }

                error = hb_mc_tile_unfreeze(device->mc, &tiles[tile_id]);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to unfreeze tile (%d,%d).\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }
        }
        return HB_MC_SUCCESS;
}





/*! 
 * Sets a Vanilla Core's binary symbol to the desired value
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] symbol     Symbol to be set in tile's binary
 * @param[in] val        Val to set the symbol 
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
static int hb_mc_tile_set_symbol_val (hb_mc_manycore_t *mc,
                                      const hb_mc_eva_map_t *map,
                                      const unsigned char* bin,
                                      size_t bin_size,
                                      const hb_mc_coordinate_t *coord,
                                      const char* symbol,
                                      const uint32_t *val) {

        int error;

        hb_mc_eva_t symbol_eva;
        error = hb_mc_loader_symbol_to_eva(bin, bin_size, symbol, &symbol_eva); 
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to acquire %s symbol's eva.\n",
                           __func__,
                           symbol);
                return HB_MC_NOTFOUND;
        }


        error = hb_mc_manycore_eva_write (mc,
                                          map,
                                          coord,
                                          &symbol_eva,
                                          val, 4);
        if (error != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to set %s symbol for tile (%d,%d).\n",
                           __func__,
                           symbol,
                           hb_mc_coordinate_get_x(*coord),
                           hb_mc_coordinate_get_y(*coord)); 
                return error;
        }
        bsg_pr_dbg("%s: Setting tile (%d,%d) %s symbol (eva 0x%08" PRIx32 ") to 0x%08" PRIx32 ".\n",
                   __func__,
                   hb_mc_coordinate_get_x(*coord),
                   hb_mc_coordinate_get_y(*coord),
                   symbol,
                   symbol_eva,
                   *val);

        return HB_MC_SUCCESS;
}





/**
 * Sends packets to all tiles in the list to set their configuration symbols in binary 
 * Symbols include: __bsg_x/y, __bsg_id, __bsg_grp_org_x/y, CSR_TGO_X/Y registers,
 * __bsg_tile_group_id_x/y, __bsg_grid_dim_x/y 
 * @param[in]  device        Pointer to device
 * @param[in]  map           EVA to NPA mapping for tiles 
 * @param[in]  origin        Origin  coordinates of the tiles in the list
 * @param[in]  tg_id         Tile group id of the tiles in the list
 * @param[in]  tg_dim        Tile group dimensions of the tiles in the list
 * @param[in]  grid_dim      Grid dimensions of the tiles in the list
 * @param[in]  tiles         List of tile coordinates to set symbols 
 * @param[in]  num_tiles     Number of tiles in the list
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_tiles_set_config_symbols (hb_mc_device_t *device,
                                                  const hb_mc_eva_map_t *map, 
                                                  hb_mc_coordinate_t origin,
                                                  hb_mc_coordinate_t tg_id,
                                                  hb_mc_dimension_t tg_dim, 
                                                  hb_mc_dimension_t grid_dim,
                                                  const hb_mc_coordinate_t *tiles,
                                                  uint32_t num_tiles) { 

        int error;

        for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) { 
                
                hb_mc_coordinate_t coord = hb_mc_coordinate_get_relative (origin, tiles[tile_id]); 

                error = hb_mc_tile_set_origin_registers(device->mc,
                                                        &(tiles[tile_id]),
                                                        &origin);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin registers CSR_TGO_X/Y.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }



                // Set tile's tile group origin __bsg_grp_org_x/y symbols.
                hb_mc_idx_t origin_x = hb_mc_coordinate_get_x (origin); 
                hb_mc_idx_t origin_y = hb_mc_coordinate_get_y (origin); 
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_grp_org_x",
                                                  &origin_x);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_grp_org_x symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }


                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_grp_org_y",
                                                  &origin_y);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_grp_org_y symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }




                // Set tile's index __bsg_x/y symbols.
                // A tile's __bsg_x/y symbols represent its X/Y 
                // coordinates with respect to the origin tile 
                hb_mc_idx_t coord_x = hb_mc_coordinate_get_x (coord); 
                hb_mc_idx_t coord_y = hb_mc_coordinate_get_y (coord); 
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_x",
                                                  &coord_x);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_x symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }


                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_y",
                                                  &coord_y);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_y symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }




                // Set tile's __bsg_id symbol.
                // bsg_id uniquely identifies each tile in a tile group
                // Tile group is a 2D array of tiles that concurrently run a kernel inside the binary
                // __bsg_x/y represent the X/Y coordiantes of tile relative to tile group origin
                // The flat tile id is calculated using tile group dimensions 
                // and the tile group X/Y coordiantes relative to tile group origin as follows:
                // __bsg_id = __bsg_y * __bsg_tile_group_dim_x + __bsg_x
                hb_mc_idx_t id = hb_mc_coordinate_get_y(coord) * hb_mc_dimension_get_x(tg_dim) + hb_mc_coordinate_get_x(coord); 
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_id",
                                                  &id);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_id symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }
        



                // Set tile's tile group index __bsg_tile_group_id_x/y symbols.
                // Grid is a 2D array of tile groups representing an application
                // bsg_tile_group_id uniquely identifies each tile group in a grid.
                // The flat tile group id is calculated using the grid dimensions
                // and the tile group X/Y coordinates relative to grid origin as follows:
                // __bsg_tile_group_id = __bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x
                // bsg_tile_group_id is used in bsg_print_stat to distinguish between
                // unique tile group invocations that have been executed in sequence on the same tile(s).
                hb_mc_idx_t tg_id_x = hb_mc_coordinate_get_x (tg_id); 
                hb_mc_idx_t tg_id_y = hb_mc_coordinate_get_y (tg_id);
                hb_mc_idx_t tg_id = tg_id_y * hb_mc_dimension_get_x(grid_dim) + tg_id_x;
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_tile_group_id_x",
                                                  &tg_id_x);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_tile_group_id_x symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }


                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_tile_group_id_y",
                                                  &tg_id_y);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_tile_group_id_y symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }


                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_tile_group_id",
                                                  &tg_id);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_tile_group_id symbol.\n",
                                    __func__,
                                    hb_mc_coordinate_get_x(tiles[tile_id]),
                                    hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }



                // Set tile's grid dimension __bsg_grid_dim_x/y symbol.
                hb_mc_idx_t grid_dim_x = hb_mc_dimension_get_x (grid_dim); 
                hb_mc_idx_t grid_dim_y = hb_mc_dimension_get_y (grid_dim);
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_grid_dim_x",
                                                  &grid_dim_x);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_grid_dim_x symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }


                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "__bsg_grid_dim_y",
                                                  &grid_dim_y);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) __bsg_grid_dim_y symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }



                // Set tile's finish signal value  cuda_finish_signal_val symbol.
                uint32_t finish_signal_val = HB_MC_CUDA_FINISH_SIGNAL_VAL;
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_finish_signal_val",
                                                  &finish_signal_val);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_finish_signal_val symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }




                // Set tile's kernel not loaded value  cuda_kernel_not_loaded_val symbol.
                uint32_t kernel_not_loaded_val = HB_MC_CUDA_KERNEL_NOT_LOADED_VAL;
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_kernel_not_loaded_val",
                                                  &kernel_not_loaded_val);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_kernel_not_loaded_val symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }
        }

        return HB_MC_SUCCESS; 
}
                                        
                                        



/**
 * Sends packets to all tiles in the list to set their runtime symbols in binary 
 * Symbols include: cuda_kernel_ptr, cuda_argc, cuda_argv_ptr, cuda_finish_signal_addr
 * @param[in]  device        Pointer to device
 * @param[in]  map           EVA to NPA mapping for tiles 
 * @param[in]  argc          Kernel's argument count for cuda_argc symbol
 * @param[in]  args_eva      Kernel's pointer to argument list for cuda_argv_ptr symbol
 * @param[in]  finish_signal_npa   Kernel's finish signal npa
 * @param[in]  kernel_eva    EVA address of kernel on DRAM for cuda_kernel_ptr symbols
 * @param[in]  tiles         List of tile coordinates to set symbols 
 * @param[in]  num_tiles     Number of tiles in the list
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_device_tiles_set_runtime_symbols (     hb_mc_device_t *device,
                                                        const hb_mc_eva_map_t *map, 
                                                        uint32_t argc, 
                                                        hb_mc_eva_t args_eva,
                                                        hb_mc_npa_t finish_signal_npa, 
                                                        hb_mc_eva_t kernel_eva, 
                                                        const hb_mc_coordinate_t *tiles,
                                                        uint32_t num_tiles) { 
        int error;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config (device->mc); 


        for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) { 


                // Set tile's argument count cuda_argc symbol.
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_argc",
                                                  &argc);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_argc symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }




                // Set tile's pointer to argument list cuda_argv_ptr symbol.
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_argv_ptr",
                                                  &args_eva);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_argv_ptr symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }




                // Set tile's pointer to argument list cuda_argv_ptr symbol.
                // Calculate the eva address to which the tile is supposed to send it's finish signal
                hb_mc_eva_t finish_signal_eva;
                size_t sz; 
                error = hb_mc_npa_to_eva (device->mc, map, &(tiles[tile_id]), &(finish_signal_npa), &finish_signal_eva, &sz); 
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to acquire finish signal address eva from npa.\n", __func__); 
                        return error;
                }

                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_finish_signal_addr",
                                                  &finish_signal_eva);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_finish_signal_addr symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }




                // Finally, set tile's pointer to kernel cuda_kernel_ptr symbol.
                error = hb_mc_tile_set_symbol_val(device->mc,
                                                  map,
                                                  device->program->bin,
                                                  device->program->bin_size,
                                                  &(tiles[tile_id]),
                                                  "cuda_kernel_ptr",
                                                  &kernel_eva);
                if (error != HB_MC_SUCCESS) { 
                        bsg_pr_err("%s: failed to set tile (%d,%d) cuda_kernel_ptr symbol.\n",
                                   __func__,
                                   hb_mc_coordinate_get_x(tiles[tile_id]),
                                   hb_mc_coordinate_get_y(tiles[tile_id]));
                        return error;
                }
        }

        return HB_MC_SUCCESS;
}






