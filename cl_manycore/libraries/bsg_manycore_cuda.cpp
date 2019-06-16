#include <bsg_manycore_cuda.h>  
#include <bsg_manycore_driver.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_memory_manager.h>
#include <bsg_manycore_elf.h>
#include <bsg_manycore_mem.h>
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


static hb_mc_epa_t hb_mc_tile_group_get_finish_signal_addr(hb_mc_tile_group_t *tg);  

static hb_mc_coordinate_t hb_mc_get_relative_coordinate (hb_mc_coordinate_t origin, hb_mc_coordinate_t coord); 

static hb_mc_idx_t hb_mc_coordinate_to_index (hb_mc_coordinate_t coord, hb_mc_dimension_t dim); 

static int  hb_mc_dimension_to_length (hb_mc_dimension_t dim); 

static hb_mc_idx_t hb_mc_get_tile_id (hb_mc_coordinate_t origin, hb_mc_dimension_t dim, hb_mc_coordinate_t coord); 


__attribute__((warn_unused_result))
static int hb_mc_tile_group_exit (hb_mc_tile_group_t *tg); 


__attribute__((warn_unused_result))
static int hb_mc_tile_group_kernel_exit (hb_mc_kernel_t *kernel); 

__attribute__((warn_unused_result))
static int hb_mc_tile_group_kernel_init (	hb_mc_tile_group_t *tg, 
						char* name, 
						uint32_t argc, 
						uint32_t argv[]); 






/*!
 * Gets the x coordinates of a list of hb_mc_tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_x(hb_mc_tile_t *tiles, uint8_t *x_list, uint32_t num_tiles) {
	if (!tiles || !x_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		x_list[i] = hb_mc_coordinate_get_x(tiles[i].coord);
	}
	return HB_MC_SUCCESS;
}




/*!
 * Gets the x coordinates of a list of hb_mc_tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_y(hb_mc_tile_t *tiles, uint8_t *y_list, uint32_t num_tiles) {
	if (!tiles || !y_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		y_list[i] = hb_mc_coordinate_get_y(tiles[i].coord);
	}
	return HB_MC_SUCCESS;
}





/**
 * Takes in a hb_mc_device_t struct and initializes a mesh of tile in the Manycore device.
 * @param[in]  device        Pointer to device
 * @parma[in]  dim           X/Y dimensions of the tile pool (mesh) to be initialized
 * @return HB_MC_SUCCESS if successful, otherwise an error code is returned. 
 */
int hb_mc_mesh_init (hb_mc_device_t *device, hb_mc_dimension_t dim){ 

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
			device->mesh->tiles[tile_id].free = 1;
		}
	}

	return HB_MC_SUCCESS;	
}




/**
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
int hb_mc_grid_init (	hb_mc_device_t *device,
			hb_mc_dimension_t grid_dim,
			hb_mc_dimension_t tg_dim,
			char* name, uint32_t argc, uint32_t argv[]) {
	int error; 
	for (hb_mc_idx_t tg_id_x = 0; tg_id_x < hb_mc_dimension_get_x(grid_dim); tg_id_x ++) { 
		for (hb_mc_idx_t tg_id_y = 0; tg_id_y < hb_mc_dimension_get_y(grid_dim); tg_id_y ++) { 
			hb_mc_coordinate_t tg_id = hb_mc_coordinate(tg_id_x, tg_id_y);
			error = hb_mc_tile_group_enqueue(device, device->num_grids, tg_id, grid_dim, tg_dim, name, argc, argv); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err(	"%s: failed to initialize tile group (%d,%d) of grid %d.\n",
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
 * Checks if a groups of tiles starting from a specific origin are all free or not.
 * @param[in]  device        Pointer to device
 * @param[in]  origin        Origin of the group of tiles to check for availability
 * @param[in]  dim           Dimension of the group of tiles in question
 * @return HB_MC_SUCCESS if all tiles in the group are free, otherwise an error code is returned. 
 */
static int hb_mc_device_tiles_are_free (hb_mc_device_t *device, hb_mc_coordinate_t origin, hb_mc_dimension_t dim) { 
	if (	hb_mc_coordinate_get_x(origin) + hb_mc_dimension_get_x(dim) > 
		hb_mc_coordinate_get_x(device->mesh->origin) + hb_mc_dimension_get_x(device->mesh->dim)) { 
		bsg_pr_err (	"%s: a %dx%d tile group starting from origin (%d,%d) \
				 does not fit in %dx%d device mesh. Check X dimension or origin.\n",
				__func__, 
				hb_mc_dimension_get_x(dim), hb_mc_dimension_get_y(dim),
				hb_mc_coordinate_get_x(origin), hb_mc_coordinate_get_y(origin), 
				hb_mc_dimension_get_x(device->mesh->dim), hb_mc_dimension_get_y(device->mesh->dim)); 
		return HB_MC_INVALID;
	}
				
	if (	hb_mc_coordinate_get_y(origin) + hb_mc_dimension_get_y(dim) >
		hb_mc_coordinate_get_y(device->mesh->origin) + hb_mc_dimension_get_y(device->mesh->dim)) { 
		bsg_pr_err (	"%s: a %dx%d tile group starting from origin (%d,%d) \
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
			if (!device->mesh->tiles[tile_id].free) 
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
static int hb_mc_tile_group_initialize_tiles (	hb_mc_device_t *device,
						hb_mc_tile_group_t *tg,
						hb_mc_coordinate_t origin) { 

	int error;

	tg->origin = origin;

	error = hb_mc_origin_eva_map_init (tg->map, origin); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n",
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
	for (hb_mc_idx_t x = hb_mc_coordinate_get_x(origin); x < hb_mc_coordinate_get_x(origin) + hb_mc_dimension_get_x(tg->dim); x++){
		for (hb_mc_idx_t y = hb_mc_coordinate_get_y(origin); y < hb_mc_coordinate_get_y(origin) + hb_mc_dimension_get_y(tg->dim); y++){
			hb_mc_idx_t device_tile_id = hb_mc_get_tile_id (device->mesh->origin, device->mesh->dim, hb_mc_coordinate(x, y));

			device->mesh->tiles[device_tile_id].origin = origin;
			device->mesh->tiles[device_tile_id].tile_group_id = tg->id;
			device->mesh->tiles[device_tile_id].free = 0;


			tile_list[tg_tile_id] = hb_mc_coordinate (x, y); 
			tg_tile_id ++;
		}
	}




	// Set the configuration symbols of all tiles inside tile group
	error = hb_mc_device_tiles_set_symbols(	device,
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
int hb_mc_tile_group_allocate_tiles (hb_mc_device_t *device, hb_mc_tile_group_t *tg){
	int error;
	if (hb_mc_dimension_get_x(tg->dim) > hb_mc_dimension_get_x(device->mesh->dim)){
		bsg_pr_err(	"%s: tile group X dimension (%d) larger than mesh X dimension (%d).\n",
				__func__,
				hb_mc_dimension_get_x(tg->dim),
				hb_mc_dimension_get_x(device->mesh->dim));
		return HB_MC_INVALID;
	}
	if (hb_mc_dimension_get_y(tg->dim) > hb_mc_dimension_get_y(device->mesh->dim)){
		bsg_pr_err(	"%s: tile group Y dimension (%d) larger than mesh Y dimension (%d).\n",
				__func__,
				hb_mc_dimension_get_y(tg->dim),
				hb_mc_dimension_get_y(device->mesh->dim));
		return HB_MC_INVALID;
	}

	hb_mc_coordinate_t tile_coord;
	hb_mc_idx_t tile_id; 

	// Iterate over the entire mesh as tile (org_y, org_x) being the origin of the new tile group to allcoate 
	for (	hb_mc_idx_t org_y = hb_mc_coordinate_get_y(device->mesh->origin);
		 org_y <= (hb_mc_coordinate_get_y(device->mesh->origin)
		 + hb_mc_dimension_get_y(device->mesh->dim)
		 - hb_mc_dimension_get_y(tg->dim)); org_y++){
		for (	hb_mc_idx_t org_x = hb_mc_coordinate_get_x(device->mesh->origin);
			 org_x <= (hb_mc_coordinate_get_x(device->mesh->origin)
			 + hb_mc_dimension_get_x(device->mesh->dim)
			 - hb_mc_dimension_get_x(tg->dim)); org_x++){


			// Search if a tg->dim.x * tg->dim.y group of tiles starting from (org_x,org_y) are all free
			if (hb_mc_device_tiles_are_free(device, hb_mc_coordinate(org_x, org_y), tg->dim) == HB_MC_SUCCESS) { 

				// Found a free group of tiles at origin (org_x, org_y), now initialize
				// all these tiles by sending packets and claiming them for this tile group
				error = hb_mc_tile_group_initialize_tiles (device, tg, hb_mc_coordinate(org_x, org_y));
				if (error != HB_MC_SUCCESS) { 
					bsg_pr_err(	"%s: failed to initialize mesh tiles for grid %d tile group (%d,%d).\n",
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
int hb_mc_tile_group_enqueue (	hb_mc_device_t* device,
				grid_id_t grid_id,
				hb_mc_coordinate_t tg_id,
				hb_mc_dimension_t grid_dim,
				hb_mc_dimension_t dim,
				char* name, uint32_t argc, uint32_t argv[]) {

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
		bsg_pr_err(	"%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n",
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
static int hb_mc_tile_group_kernel_init (	hb_mc_tile_group_t *tg, 
						char* name, 
						uint32_t argc, 
						uint32_t argv[]) {
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
	tg->kernel->argv = (uint32_t *) malloc (tg->kernel->argc * sizeof(uint32_t)); 
	if (tg->kernel->argv == NULL) { 
		bsg_pr_err("%s: failed to allocate space on devcie for kernel's argument list.\n", __func__); 
		return HB_MC_NOMEM;
	}
	memcpy (tg->kernel->argv, argv, argc * sizeof(uint32_t)); 	
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
int hb_mc_tile_group_launch (hb_mc_device_t *device, hb_mc_tile_group_t *tg) {

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (device->mc); 

	hb_mc_eva_t args_eva;

	// allocate device memory for arguments
	error = hb_mc_device_malloc (device, (tg->kernel->argc) * sizeof(uint32_t), &args_eva);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err(	"%s: failed to allocate space on device for grid %d tile group (%d,%d) arguments.\n",
				__func__,
				tg->grid_id,
				hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
		return HB_MC_NOMEM;
	}

	// transfer the arguments to dram
	error = hb_mc_device_memcpy(	device, reinterpret_cast<void *>(args_eva),
					 (void *) &(tg->kernel->argv[0]),
					 (tg->kernel->argc) * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err(	"%s: failed to copy grid %d tile group (%d,%d) arguments to device.\n",
				__func__,
				tg->grid_id, hb_mc_coordinate_get_x(tg->id),
				hb_mc_coordinate_get_y(tg->id)); 
		return error;
	}
	
	hb_mc_eva_t kernel_eva; 
	error = hb_mc_loader_symbol_to_eva (device->program->bin, device->program->bin_size, tg->kernel->name, &kernel_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: invalid kernel name %s for grid %d tile group (%d,%d).\n",
				__func__,
				tg->kernel->name,
				tg->grid_id,
				hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
		return error;
	}	

	hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 
	hb_mc_npa_t finish_signal_npa = hb_mc_npa(host_coordinate, tg->kernel->finish_signal_addr); 


	hb_mc_idx_t tile_id;
	for (	int y = hb_mc_coordinate_get_y(tg->origin);
		y < hb_mc_coordinate_get_y(tg->origin) + hb_mc_dimension_get_y(tg->dim); y++){
		for (	int x = hb_mc_coordinate_get_x(tg->origin);
			x < hb_mc_coordinate_get_x(tg->origin) + hb_mc_dimension_get_x(tg->dim); x++){

			tile_id = hb_mc_get_tile_id (device->mesh->origin, device->mesh->dim, hb_mc_coordinate(x, y)); 


			error = hb_mc_tile_set_argc_symbol(	device->mc, tg->map, 
								device->program->bin,
								device->program->bin_size,
								&(device->mesh->tiles[tile_id].coord),
								&(tg->kernel->argc));
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err(	"%s: failed to write argc to tile (%d,%d) for grid %d tile group (%d,%d).\n",
						__func__,
						hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord),
						hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord),
						tg->grid_id,
						hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
				return error;
			}
			bsg_pr_dbg(	"%s: Setting tile[%d] (%d,%d) argc to %d.\n",
					__func__,
					tile_id,
					x, y,
					tg->kernel->argc);


			error = hb_mc_tile_set_argv_ptr_symbol(	device->mc, tg->map, 
								device->program->bin,
								device->program->bin_size,
								&(device->mesh->tiles[tile_id].coord),
								&args_eva);
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err(	"%s: failed to write argv pointer to tile (%d,%d) for grid %d tile group (%d,%d).\n",
						__func__,
						hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord),
						hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord),
						tg->grid_id, hb_mc_coordinate_get_x(tg->id),
						hb_mc_coordinate_get_y(tg->id));
				return error;
			}
			bsg_pr_dbg(	"%s: Setting tile[%d] (%d,%d) argv to 0x%08" PRIx32 ".\n",
					__func__,
					tile_id,
					x, y,
					args_eva);



			hb_mc_eva_t finish_signal_eva;
			size_t sz; 
			error = hb_mc_npa_to_eva (cfg, tg->map, &(device->mesh->tiles[tile_id].coord), &(finish_signal_npa), &finish_signal_eva, &sz); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to acquire finish signal address eva from npa.\n", __func__); 
				return error;
			}




			error = hb_mc_tile_set_finish_signal_addr_symbol(	device->mc, tg->map, 
										device->program->bin,
										device->program->bin_size,
										&(device->mesh->tiles[tile_id].coord),
										&finish_signal_eva);
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err(	"%s: failed to write finish signal address to tile (%d,%d) for grid %d tile group (%d,%d).\n",
						__func__,
						hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord),
						hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord),
						tg->grid_id,
						hb_mc_coordinate_get_x(tg->id),
						hb_mc_coordinate_get_y(tg->id));
				return error;
			}
			bsg_pr_dbg(	"%s: Setting tile[%d] (%d,%d) HB_MC_CUDA_TILE_FINISH_SIGNAL_PTR_EPA to 0x%08" PRIx32 ".\n",
					__func__,
					tile_id,
					x, y,
					finish_signal_eva);



			error = hb_mc_tile_set_kernel_ptr_symbol(	device->mc, tg->map, 
									device->program->bin,
									device->program->bin_size,
									&(device->mesh->tiles[tile_id].coord),
									&kernel_eva);
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err(	"%s: failed to write kernel pointer to tile (%d,%d) for grid %d tile group (%d,%d).\n",
						__func__,
						hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord),
						hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord),
						tg->grid_id,
						hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
				return error;
			}
			bsg_pr_dbg(	"%s: Setting tile[%d] (%d,%d) HB_MC_CUDA_TILE_KERNEL_PTR_EPA to 0x%08" PRIx32 ".\n",
					__func__,
					tile_id,
					x, y,
					kernel_eva); 
		}
	} 

	tg->status=HB_MC_TILE_GROUP_STATUS_LAUNCHED;
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) launched at origin (%d,%d).\n",
			__func__, tg->grid_id,
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
int hb_mc_tile_group_deallocate_tiles(hb_mc_device_t *device, hb_mc_tile_group_t *tg) {
	int error;
	hb_mc_idx_t tile_id; 
	for (	int x = hb_mc_coordinate_get_x(tg->origin);
		x < hb_mc_coordinate_get_x(tg->origin) + hb_mc_dimension_get_x(tg->dim); x++){
		for (	int y = hb_mc_coordinate_get_y(tg->origin);
			y < hb_mc_coordinate_get_y(tg->origin) + hb_mc_dimension_get_y(tg->dim); y++){
			tile_id = hb_mc_get_tile_id (device->mesh->origin, device->mesh->dim, hb_mc_coordinate (x, y));  
			
			device->mesh->tiles[tile_id].origin = device->mesh->origin;
			device->mesh->tiles[tile_id].tile_group_id = hb_mc_coordinate( 0, 0);
			device->mesh->tiles[tile_id].free = 1;
		}
	}
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) de-allocated at origin (%d,%d).\n",
			__func__,
			tg->grid_id,
			hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
			hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
			hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));
	
	tg->status = HB_MC_TILE_GROUP_STATUS_FINISHED;

	error = hb_mc_tile_group_exit(tg); 
	if ( error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to remove tile group struct.\n", __func__);
		return error;
	}

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

	const char* name;
	uint32_t *argv;

	// Free name
	name = kernel->name;
	if (!name) { 
		bsg_pr_err("%s: calling exit on kernel with null name.\n", __func__);
		return HB_MC_INVALID;
	} else {
		free ((void *) name); 
		kernel->name = NULL;
	}


	// Free argv
	argv = kernel->argv;
	if (!argv) { 
		bsg_pr_err("%s: calling exit on kernel with null argv.\n", __func__);
		return HB_MC_INVALID;
	} else { 
		free (argv); 
		kernel->argv = NULL;
	}
	
	return HB_MC_SUCCESS;
}






/**
 * Initializes the manycore struct, and a mesh structure inside
 * device struct with list of tiles and their coordinates 
 * @param[in]  device        Pointer to device
 * @param[in]  name          Device name
 * @param[in]  id            Device id
 * @param[in]  dim           Tile pool (mesh) dimensions
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned. 
 */
int hb_mc_device_init (	hb_mc_device_t *device,
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
	

	error = hb_mc_mesh_init(device, dim);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to initialize mesh.\n", __func__);
		return HB_MC_UNINITIALIZED;
	}


	device->tile_group_capacity = 1;
	device->tile_groups = (hb_mc_tile_group_t *) malloc (device->tile_group_capacity * sizeof(hb_mc_tile_group_t));
	if (device->tile_groups == NULL) {
		bsg_pr_err("%s: failed to allocated space for list of tile groups.\n", __func__);
		return HB_MC_NOMEM;
	}
	memset (device->tile_groups, 0, device->tile_group_capacity * sizeof(hb_mc_tile_group_t));
	device->num_tile_groups = 0;
	device->num_grids = 0;

	return HB_MC_SUCCESS;
}




/**
 * Loads the binary in a device's hb_mc_program_t struct
 * onto all tiles in device's hb_mc_mesh_t struct. 
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_program_load (hb_mc_device_t *device) { 
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
	error = hb_mc_loader_load (	device->program->bin,
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

	error = hb_mc_device_tiles_set_symbols(	device,
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
	error = hb_mc_device_tiles_unfreeze(device, tile_list, num_tiles); 
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
int hb_mc_device_program_init_binary (	hb_mc_device_t *device, 
					char *bin_name,
					unsigned char* bin_data, 
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


	device->program->bin = bin_data;

	device->program->bin_size = bin_size;


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
int hb_mc_device_program_init (	hb_mc_device_t *device,
				char *bin_name,
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


	error = hb_mc_device_program_init_binary (	device, bin_name, 
							bin_data, bin_size, 
							alloc_name, id); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to initialize device with program binary.\n", __func__);
		return error;
	}

	return HB_MC_SUCCESS;
}





/**
 * Initializes program's memory allocator and creates a memory manager
 * @param[in]  program       Pointer to program
 * @param[in]  id            Id of program's meomry allocator
 * @param[in]  name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_program_allocator_init (	const hb_mc_config_t *cfg,
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
 * Checks to see if all tile groups in a device are finished.
 * @param[in]  device        Pointer to device
 * returns HB_MC_SUCCESS if all tile groups are finished, and HB_MC_FAIL otherwise.
 */
int hb_mc_device_all_tile_groups_finished(hb_mc_device_t *device) {
	
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
int hb_mc_device_wait_for_tile_group_finish_any(hb_mc_device_t *device) {
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
				hb_mc_request_packet_set_data(&finish, 0x1 /* TODO: Hardcoded */);
				hb_mc_request_packet_set_mask(&finish, HB_MC_PACKET_REQUEST_MASK_WORD);
				hb_mc_request_packet_set_op(&finish, HB_MC_PACKET_OP_REMOTE_STORE);
				hb_mc_request_packet_set_addr(&finish, tg->kernel->finish_signal_addr >> 2);

				if (hb_mc_request_packet_equals(&recv, &finish) == HB_MC_SUCCESS) {
		
					bsg_pr_dbg(	"%s: Finish packet received for grid %d tile group (%d,%d): \
							src (%d,%d), dst (%d,%d), addr: 0x%08" PRIx32 ", data: %d.\n", 
							__func__, 
							tg->grid_id, 
							hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id), 
							recv.x_src, recv.y_src, 
							recv.x_dst, recv.y_dst, 
							recv.addr, recv.data);

					error = hb_mc_tile_group_deallocate_tiles(device, tg);
					if (error != HB_MC_SUCCESS) { 
						bsg_pr_err(	"%s: failed to deallocate grid %d tile group (%d,%d).\n",
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
	

	hb_mc_manycore_exit(device->mc); 

	if (device->program->allocator->memory_manager)
		delete((awsbwhal::MemoryManager*)device->program->allocator->memory_manager);

	free (device->mc); 
	free (device->mesh);
	free (device->tile_groups);
	free (device->program);

	return HB_MC_SUCCESS;
}




/**
 * Allocates memory on device DRAM
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
 * @parma[in]  src           EVA address of src 
 * @param[in]  name          EVA address of dst
 * @param[in]  count         Size of buffer to be copied
 * @param[in]  hb_mc_memcpy_kind         Direction of copy (HB_MC_MEMCPY_TO_DEVICE / HB_MC_MEMCPY_TO_HOST)
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_memcpy (hb_mc_device_t *device, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind) {

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
		bsg_pr_err("%s: invalid copy type. Copy type can be one of HB_MC_MEMCPY_TO_DEVICE or HB_MC_MEMCPY_TO_HOST.\n", __func__);
		return HB_MC_INVALID; 
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
	hb_mc_epa_t finish_addr = 	HB_MC_CUDA_HOST_FINISH_SIGNAL_BASE_ADDR
					+ ((hb_mc_coordinate_get_y(tg->id) * hb_mc_dimension_get_x(tg->grid_dim)
					+ hb_mc_coordinate_get_x(tg->id)) << 2); /* TODO: Hardcoded */
	return finish_addr;
}




/**
 * Calculates and returns the relative coordinates based on absolute coordinates and origin coordinates 
 * @param[in]  origin        Origin coordinates 
 * @parma[in]  coord         Absolute coordinates 
 * @return     relative_coord
 */
static hb_mc_coordinate_t hb_mc_get_relative_coordinate (hb_mc_coordinate_t origin, hb_mc_coordinate_t coord) {
	hb_mc_coordinate_t relative_coord = hb_mc_coordinate ( 	hb_mc_coordinate_get_x (coord) - hb_mc_coordinate_get_x (origin) , 
								hb_mc_coordinate_get_y (coord) - hb_mc_coordinate_get_y (origin) );
	return relative_coord;
}




/**
 * Calculates and returns a 1D flat index based on 2D coordinates and 2D dimensions 
 * @param[in]  coord         2D coordinates  
 * @parma[in]  dim           Dimensions 
 * @return     idx
 */
static hb_mc_idx_t hb_mc_coordinate_to_index (hb_mc_coordinate_t coord, hb_mc_dimension_t dim) {
	hb_mc_idx_t idx = hb_mc_coordinate_get_y(coord) * hb_mc_dimension_get_x(dim) + hb_mc_coordinate_get_x(coord); 
	return idx;
} 




/**
 * Calculates and returns a 1D length based on 2D dimensions 
 * @parma[in]  dim           Dimensions 
 * @return     1D flat length
 */
static int  hb_mc_dimension_to_length (hb_mc_dimension_t dim) { 
	return (hb_mc_dimension_get_x(dim) * hb_mc_dimension_get_y(dim)); 
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
	hb_mc_coordinate_t relative_coord = hb_mc_get_relative_coordinate (origin, coord); 
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
int hb_mc_device_tiles_freeze (	hb_mc_device_t *device,
				hb_mc_coordinate_t *tiles,
				uint32_t num_tiles) { 
	int error;
	for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) { 
		error = hb_mc_tile_freeze(device->mc, &tiles[tile_id]); 
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to freeze tile (%d,%d).\n",
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
 * @param[in]  tiles         List of tile coordinates to unfreeze
 * @param[in]  num_tiles     Number of tiles in the list
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_device_tiles_unfreeze (	hb_mc_device_t *device,
					hb_mc_coordinate_t *tiles,
					uint32_t num_tiles) { 
	int error;
	for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) {
		hb_mc_npa_t kernel_ptr_npa = hb_mc_npa (tiles[tile_id], HB_MC_CUDA_TILE_KERNEL_PTR_EPA); 
		error = hb_mc_manycore_write32(device->mc, &kernel_ptr_npa, HB_MC_CUDA_KERNEL_NOT_LOADED);
		if (error != HB_MC_SUCCESS) {
			bsg_pr_err(	"%s: failed to initialize kernel register to 0x1 in tile (%d,%d).\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}

		error = hb_mc_tile_unfreeze(device->mc, &tiles[tile_id]);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to unfreeze tile (%d,%d).\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}
	}
	return HB_MC_SUCCESS;
}







/**
 * Sends packets to all tiles in the list to set their configuration symbols in binary  
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
int hb_mc_device_tiles_set_symbols (	hb_mc_device_t *device,
					hb_mc_eva_map_t *map, 
					hb_mc_coordinate_t origin,
					hb_mc_coordinate_t tg_id,
					hb_mc_dimension_t tg_dim, 
					hb_mc_dimension_t grid_dim,
					hb_mc_coordinate_t *tiles,
					uint32_t num_tiles) { 

	int error;

	for (hb_mc_idx_t tile_id = 0; tile_id < num_tiles; tile_id ++) { 
		
		hb_mc_coordinate_t coord = hb_mc_get_relative_coordinate (origin, tiles[tile_id]); 

		error = hb_mc_tile_set_origin_registers(device->mc,
							&(tiles[tile_id]),
							&origin);
		if (error != HB_MC_SUCCESS) { 
			 bsg_pr_err(	"%s: failed to set tile (%d,%d) tile group origin registers CSR_TGO_X/Y.\n",
					 __func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}



		error = hb_mc_tile_set_origin_symbols(	device->mc, map, device->program->bin,
							device->program->bin_size,
							&(tiles[tile_id]),
							&origin);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}


		error = hb_mc_tile_set_coord_symbols(	device->mc, map,
							device->program->bin, device->program->bin_size,
							&(tiles[tile_id]), &coord);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}


		error = hb_mc_tile_set_id_symbol(	device->mc, map,
							device->program->bin, device->program->bin_size,
							&(tiles[tile_id]),
							&(coord), &(tg_dim));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to set tile (%d,%d) id symbol __bsg_id.\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}


		error = hb_mc_tile_set_tile_group_id_symbols(	device->mc, map,
								device->program->bin, device->program->bin_size,
								&(tiles[tile_id]), &(tg_id));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}


		error = hb_mc_tile_set_grid_dim_symbols(device->mc, map,
							device->program->bin, device->program->bin_size,
							&(tiles[tile_id]), &(grid_dim));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err(	"%s: failed to set tile (%d,%d) grid size symbol __bsg_grid_size.\n",
					__func__,
					hb_mc_coordinate_get_x(tiles[tile_id]),
					hb_mc_coordinate_get_y(tiles[tile_id]));
			return error;
		}
	}

	return HB_MC_SUCCESS; 
}
					
					

 

/*!
 * Sets a Vanilla Core Endpoint's tile group's origin symbols __bsg_grp_org_x/y.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file.
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the origin of.
 * @param[in] origin     Origin coordinates.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin_symbols (	hb_mc_manycore_t *mc,
					hb_mc_eva_map_t *map,
					unsigned char* bin,
					size_t bin_size,
					const hb_mc_coordinate_t *coord,
					const hb_mc_coordinate_t *origin){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t org_x_eva, org_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grp_org_x", &org_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_grp_org_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grp_org_y", &org_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_grp_org_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_idx_t origin_x = hb_mc_coordinate_get_x(*origin); 
	hb_mc_idx_t origin_y = hb_mc_coordinate_get_y(*origin); 

	error = hb_mc_manycore_eva_write (mc, map, coord, &org_x_eva, &origin_x, 4); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_grp_org_x to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_grp_org_x (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			org_x_eva,
			hb_mc_coordinate_get_x(*origin));



	error = hb_mc_manycore_eva_write (mc, map, coord, &org_y_eva, &origin_y, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_grp_org_y to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_grp_org_y (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			org_y_eva,
			hb_mc_coordinate_get_y(*origin));

	return HB_MC_SUCCESS;
}




/*!
 * Sets a Vanilla Core Endpoint's tile group's coordinate symbols __bsg_x/y.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the coordinates of.
 * @param[in] coord_val  The coordinates to set the tile.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_coord_symbols (	hb_mc_manycore_t *mc,
					hb_mc_eva_map_t *map,
					unsigned char* bin,
					size_t bin_size,
					const hb_mc_coordinate_t *coord,
					const hb_mc_coordinate_t *coord_val){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t bsg_x_eva, bsg_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_x", &bsg_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_y", &bsg_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_idx_t coord_val_x = hb_mc_coordinate_get_x (*coord_val);
	hb_mc_idx_t coord_val_y = hb_mc_coordinate_get_y (*coord_val);

	error = hb_mc_manycore_eva_write (mc, map, coord, &bsg_x_eva, &coord_val_x, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_x to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg__x (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			bsg_x_eva,
			hb_mc_coordinate_get_x(*coord_val));



	error = hb_mc_manycore_eva_write (mc, map, coord, &bsg_y_eva, &coord_val_y, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_y to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_y (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			bsg_y_eva,
			hb_mc_coordinate_get_y(*coord_val));

	return HB_MC_SUCCESS;
}




/*! 
 * Sets a Vanilla Core Endpoint's tile's __bsg_id symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the id of.
 * @param[in] coord_val  The coordinates to set the tile.
 * @param[in] dim        Tile group dimensions
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_id_symbol (	hb_mc_manycore_t *mc,
				hb_mc_eva_map_t *map,
				unsigned char* bin,
				size_t bin_size,
				const hb_mc_coordinate_t *coord,
				const hb_mc_coordinate_t *coord_val,
				const hb_mc_dimension_t *dim){


	int error;

	// calculate tile's id 
	hb_mc_idx_t id = hb_mc_coordinate_get_y(*coord_val) * hb_mc_dimension_get_x(*dim) + hb_mc_coordinate_get_x(*coord_val); 

	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t bsg_id_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_id", &bsg_id_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s:: failed to acquire __bsg_id eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	error = hb_mc_manycore_eva_write (mc, map, coord, &bsg_id_eva, &id, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_id to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_id (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			bsg_id_eva, id);

	return HB_MC_SUCCESS;
}




/*! 
 * Sets a Vanilla Core Endpoint's tile's __bsg_tile_group_id_x/y symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] tg_id      Tile group id
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_tile_group_id_symbols (	hb_mc_manycore_t *mc,
						hb_mc_eva_map_t *map,
						unsigned char* bin,
						size_t bin_size,
						const hb_mc_coordinate_t *coord,
						const hb_mc_coordinate_t *tg_id){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t tg_id_x_eva, tg_id_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_tile_group_id_x", &tg_id_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_tile_group_id_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_tile_group_id_y", &tg_id_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_tile_group_id_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_idx_t tg_id_x = hb_mc_coordinate_get_x (*tg_id); 
	hb_mc_idx_t tg_id_y = hb_mc_coordinate_get_y (*tg_id); 

	error = hb_mc_manycore_eva_write (mc, map, coord, &tg_id_x_eva, &tg_id_x, 4); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_tile_group_id_x to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_tile_group_id_x (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			tg_id_x_eva,
			hb_mc_coordinate_get_x(*tg_id));



	error = hb_mc_manycore_eva_write (mc, map, coord, &tg_id_y_eva, &tg_id_y, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_tile_group_id_y to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_tile_group_id_y (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			tg_id_y_eva,
			hb_mc_coordinate_get_y(*tg_id));


	return HB_MC_SUCCESS;
}




/*! 
 * Sets a Vanilla Core Endpoint's tile's __bsg_grid_dim_x/y symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] tg_id      Grid dimensions
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_grid_dim_symbols (	hb_mc_manycore_t *mc,
					hb_mc_eva_map_t *map,
					unsigned char* bin,
					size_t bin_size,
					const hb_mc_coordinate_t *coord,
					const hb_mc_dimension_t *grid_dim){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t grid_dim_x_eva, grid_dim_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grid_dim_x", &grid_dim_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_grid_dim_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grid_dim_y", &grid_dim_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire __bsg_grid_dim_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_idx_t grid_dim_x = hb_mc_dimension_get_x (*grid_dim); 
	hb_mc_idx_t grid_dim_y = hb_mc_dimension_get_y (*grid_dim); 

	error = hb_mc_manycore_eva_write (mc, map, coord, &grid_dim_x_eva, &grid_dim_x, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_grid_dim_x to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_gird_dim_x (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			grid_dim_x_eva,
			hb_mc_dimension_get_x(*grid_dim));



	error = hb_mc_manycore_eva_write (mc, map, coord, &grid_dim_y_eva, &grid_dim_y, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write __bsg_grid_dim_y to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) __bsg_grid_dim_y (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			grid_dim_y_eva,
			hb_mc_dimension_get_y(*grid_dim));

	return HB_MC_SUCCESS;
}





/*! 
 * Sets a Vanilla Core's cuda_kernel_ptr symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] kernel_eva EVA address of the kernel 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_kernel_ptr_symbol (	hb_mc_manycore_t *mc,
					hb_mc_eva_map_t *map,
					unsigned char* bin,
					size_t bin_size,
					const hb_mc_coordinate_t *coord,
					const hb_mc_eva_t *kernel_eva){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t kernel_ptr_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "cuda_kernel_ptr", &kernel_ptr_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire cuda_kernel_ptr eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}



	error = hb_mc_manycore_eva_write (mc, map, coord, &kernel_ptr_eva, kernel_eva, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write cuda_kernel_ptr to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) cuda_kernel_ptr (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			kernel_ptr_eva,
			*kernel_eva);

	return HB_MC_SUCCESS;
}





/*! 
 * Sets a Vanilla Core's cuda_argc symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] argc       Argument count of the kernel, to be written into tile's argc symbol 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_argc_symbol (	hb_mc_manycore_t *mc,
					hb_mc_eva_map_t *map,
					unsigned char* bin,
					size_t bin_size,
					const hb_mc_coordinate_t *coord,
					const uint32_t *argc){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t argc_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "cuda_argc", &argc_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire cuda_argc eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}



	error = hb_mc_manycore_eva_write (mc, map, coord, &argc_eva, argc, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write cuda_argc to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) cuda_argc (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			argc_eva,
			*argc);

	return HB_MC_SUCCESS;
}




/*! 
 * Sets a Vanilla Core's cuda_argv_ptr symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] argv_eva   Pointer to argument list of the kernel, to be written into tile's argv_ptr symbol 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_argv_ptr_symbol (	hb_mc_manycore_t *mc,
					hb_mc_eva_map_t *map,
					unsigned char* bin,
					size_t bin_size,
					const hb_mc_coordinate_t *coord,
					const hb_mc_eva_t *argv_ptr){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t argv_ptr_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "cuda_argv_ptr", &argv_ptr_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire cuda_argv_ptr eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}



	error = hb_mc_manycore_eva_write (mc, map, coord, &argv_ptr_eva, argv_ptr, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write cuda_argv_ptr to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) cuda_argv_ptr (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			argv_ptr_eva,
			*argv_ptr);

	return HB_MC_SUCCESS;
}




/*! 
 * Sets a Vanilla Core's cuda_finish_signal_addr symbol.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] map        Eva to npa mapping. 
 * @param[in] bin        Binary elf file. 
 * @param[in] bin_size   Size of binary file. 
 * @param[in] coord      Tile coordinates to set the tile group id of.
 * @param[in] finish_signal_addr_eva   Pointer to argument list of the kernel, to be written into tile's argv_ptr symbol 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_finish_signal_addr_symbol (	hb_mc_manycore_t *mc,
						hb_mc_eva_map_t *map,
						unsigned char* bin,
						size_t bin_size,
						const hb_mc_coordinate_t *coord,
						const hb_mc_eva_t *finish_signal_addr_eva){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t finish_signal_symbol_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "cuda_finish_signal_addr", &finish_signal_symbol_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to acquire cuda_finish_signal_addr eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}



	error = hb_mc_manycore_eva_write (mc, map, coord, &finish_signal_symbol_eva, finish_signal_addr_eva, 4);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to write cuda_finish_signal_addr to tile (%d,%d).\n",
				__func__,
				hb_mc_coordinate_get_x(*coord),
				hb_mc_coordinate_get_y(*coord)); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) cuda_finish_signal_addr (eva 0x%08" PRIx32 ") to %d.\n",
			__func__,
			hb_mc_coordinate_get_x(*coord),
			hb_mc_coordinate_get_y(*coord),
			finish_signal_symbol_eva,
			*finish_signal_addr_eva);

	return HB_MC_SUCCESS;
}


