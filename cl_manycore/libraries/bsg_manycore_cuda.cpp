#include <bsg_manycore_cuda.h>  
#include <bsg_manycore_driver.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_memory_manager.h>
#include <bsg_manycore_elf.h>
#include <bsg_manycore_mem.h>
#include <bsg_manycore_loader_dep.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_origin_eva_map.h>

static const hb_mc_epa_t KERNEL_REG = 0x1000; //!< EPA of kernel. 
static const hb_mc_epa_t ARGC_REG   = 0x1004; //!< EPA of number of arguments kernel will use. 
static const hb_mc_epa_t ARGV_REG   = 0x1008; //!< EPA of arguments for kernel. 
static const hb_mc_epa_t SIGNAL_REG = 0x100c; //!< EPA of register that holds signal address. Tile will write to this address once it completes the kernel.   


static const hb_mc_epa_t FINISH_BASE_ADDR = 0xF000; //!< EPA to which tile group sends a finish packet once it finishes executing a kernel  

static uint32_t const DRAM_SIZE = 0x80000000;

static hb_mc_epa_t hb_mc_tile_group_get_finish_signal_addr(tile_group_t *tg);  

static hb_mc_coordinate_t hb_mc_get_relative_coordinate (hb_mc_coordinate_t origin, hb_mc_coordinate_t coord); 

static hb_mc_idx_t hb_mc_coordinate_to_flat_index (hb_mc_coordinate_t coord, hb_mc_dimension_t dim); 

static int  hb_mc_dimension_to_size (hb_mc_dimension_t dim); 






/*!
 * Gets the x coordinates of a list of tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_x(tile_t *tiles, uint8_t *x_list, uint32_t num_tiles) {
	if (!tiles || !x_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		x_list[i] = hb_mc_coordinate_get_x(tiles[i].coord);
	}
	return HB_MC_SUCCESS;
}




/*!
 * Gets the x coordinates of a list of tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_y(tile_t *tiles, uint8_t *y_list, uint32_t num_tiles) {
	if (!tiles || !y_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		y_list[i] = hb_mc_coordinate_get_y(tiles[i].coord);
	}
	return HB_MC_SUCCESS;
}





/**
 * Takes in a device_t struct and initializes a mesh of tile in the Manycore device.
 * @param[in]  device        Pointer to device
 * @parma[in]  dim           X/Y dimensions of the tile pool (mesh) to be initialized
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_mesh_init (device_t *device, hb_mc_dimension_t dim){ 

	int error;

	const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device->mc);
	hb_mc_dimension_t device_dim = cfg->dimension;

	
	if (hb_mc_dimension_get_x(dim) <= 0){
		bsg_pr_err("%s: Mesh X dimension (%d) not valid.\n", __func__, hb_mc_dimension_get_x(dim)); 
		return HB_MC_INVALID;
	}
	if (hb_mc_dimension_get_y(dim) <= 0){
		bsg_pr_err("%s: Mesh Y dimension (%d) not valid.\n", __func__, hb_mc_dimension_get_y(dim));
		return HB_MC_INVALID;
	}
	if (hb_mc_dimension_get_x(dim) > hb_mc_dimension_get_x(device_dim)){
		bsg_pr_err("%s: Mesh X dimension (%d) larger than device X dimension.\n", __func__, dim.x, hb_mc_dimension_get_x(device_dim)); 
		return HB_MC_INVALID;
	}
	if (hb_mc_dimension_get_y(dim) > hb_mc_dimension_get_y(device_dim)){
		bsg_pr_err("%s: Mesh Y dimension (%d) larger than device Y dimension.\n", __func__, dim.y, hb_mc_dimension_get_y(device_dim));
		return HB_MC_INVALID;
	}

	tile_t* tiles = new tile_t [hb_mc_dimension_to_size (dim)];
	for (int x = hb_mc_coordinate_get_x(default_origin); x < hb_mc_coordinate_get_x(default_origin) + hb_mc_dimension_get_x(dim); x++){
		for (int y = hb_mc_coordinate_get_y(default_origin); y < hb_mc_coordinate_get_y(default_origin) + hb_mc_dimension_get_y(dim); y++){
			hb_mc_coordinate_t relative_coord = hb_mc_get_relative_coordinate (default_origin, hb_mc_coordinate(x, y));
			hb_mc_idx_t tile_id = hb_mc_coordinate_to_flat_index (relative_coord, dim); 
			tiles[tile_id].coord = hb_mc_coordinate(x, y);
			tiles[tile_id].origin = default_origin;
			tiles[tile_id].tile_group_id = hb_mc_coordinate(-1, -1); 
			tiles[tile_id].free = 1;
		}
	}

	mesh_t *mesh = new mesh_t; 
	mesh->dim = dim;
	mesh->origin = default_origin;
	mesh->tiles= (tile_t*)tiles;

	device->mesh = mesh;

	return HB_MC_SUCCESS;	
}




/**
 * Takes the grid size, tile group dimensions, kernel name, argc, argv* and the finish signal address, calls hb_mc_tile_group_enqueue to initialize all tile groups for grid.
 * @param[in]  device        Pointer to device
 * @param[in]  grid_dim      X/Y dimensions of the grid to be initialized
 * @param[in]  tg_dim        X/Y dimensions of tile groups in grid
 * @param[in]  name          Kernel name to be executed on tile groups in grid
 * @param[in]  argc          Number of input arguments to kernel
 * @param[in]  argv          List of input arguments to kernel
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_grid_init (device_t *device, hb_mc_dimension_t grid_dim, hb_mc_dimension_t tg_dim, char* name, uint32_t argc, uint32_t argv[]) {
	int error; 
	for (hb_mc_idx_t tg_id_x = 0; tg_id_x < hb_mc_dimension_get_x(grid_dim); tg_id_x ++) { 
		for (hb_mc_idx_t tg_id_y = 0; tg_id_y < hb_mc_dimension_get_y(grid_dim); tg_id_y ++) { 
			hb_mc_coordinate_t tg_id = hb_mc_coordinate(tg_id_x, tg_id_y);
			error = hb_mc_tile_group_enqueue(device, device->num_grids, tg_id, grid_dim, tg_dim, name, argc, argv); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to initialize tile group (%d,%d) of grid %d.\n", __func__, tg_id_x, tg_id_y, device->num_grids);
				return HB_MC_UNINITIALIZED;
			}
		}
	}
	device->num_grids ++;
	return HB_MC_SUCCESS;
}




/**
 * Searches for a free tile group inside the device mesh and allocoates it, and sets the dimensions, origin, and id of tile group.
 * @param[in]  device        Pointer to device
 * @param[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_tile_group_allocate_tiles (device_t *device, tile_group_t *tg){
	int error;
	if (hb_mc_dimension_get_x(tg->dim) > hb_mc_dimension_get_x(device->mesh->dim)){
		bsg_pr_err("%s: tile group X dimension (%d) larger than mesh X dimension (%d).\n", __func__, hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_x(device->mesh->dim));
		return HB_MC_INVALID;
	}
	if (hb_mc_dimension_get_y(tg->dim) > hb_mc_dimension_get_y(device->mesh->dim)){
		bsg_pr_err("%s: tile group Y dimension (%d) larger than mesh Y dimension (%d).\n", __func__, hb_mc_dimension_get_y(tg->dim), hb_mc_dimension_get_y(device->mesh->dim));
		return HB_MC_INVALID;
	}

	// Iterate over the entire mesh as tile (org_y, org_x) being the origin of the new tile group to allcoate 
	for (hb_mc_idx_t org_y = hb_mc_coordinate_get_y(device->mesh->origin); org_y <= (hb_mc_coordinate_get_y(device->mesh->origin) + hb_mc_dimension_get_y(device->mesh->dim) - hb_mc_dimension_get_y(tg->dim)); org_y++){
		for (hb_mc_idx_t org_x = hb_mc_coordinate_get_x(device->mesh->origin); org_x <= (hb_mc_coordinate_get_x(device->mesh->origin) + hb_mc_dimension_get_x(device->mesh->dim) - hb_mc_dimension_get_x(tg->dim)); org_x++){
			int free = 1;
			hb_mc_coordinate_t tile_coord;
			hb_mc_idx_t tile_id;
			
			// Iterate over a tg->dim.x * tg->dim.y square of tiles starting from the (org_x,org_y) checking to see if all tiles are free so they can be allocated to tile group
			for (hb_mc_idx_t x = org_x; x < org_x + hb_mc_dimension_get_x(tg->dim); x++){
				for (hb_mc_idx_t y = org_y; y < org_y + hb_mc_dimension_get_y(tg->dim); y++){
					tile_coord = hb_mc_get_relative_coordinate (device->mesh->origin, hb_mc_coordinate(x, y));
					tile_id = hb_mc_coordinate_to_flat_index (tile_coord, device->mesh->dim); 
					free = free & device->mesh->tiles[tile_id].free;
				}
			}
			if (free){ // tg->dim.x * tg->dim.y group of free tiles are found at origin (org_x, org_y)
				hb_mc_coordinate_t org = hb_mc_coordinate(org_x, org_y); 
				error = hb_mc_origin_eva_map_init (tg->map, org); 
				if (error != HB_MC_SUCCESS) { 
					bsg_pr_err("%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n", __func__, tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
					return HB_MC_FAIL;
				}
				
				for (hb_mc_idx_t x = org_x; x < org_x + hb_mc_dimension_get_x(tg->dim); x++){
					for (hb_mc_idx_t y = org_y; y < org_y + hb_mc_dimension_get_y(tg->dim); y++){
						tile_coord = hb_mc_get_relative_coordinate (device->mesh->origin, hb_mc_coordinate(x, y));
						tile_id = hb_mc_coordinate_to_flat_index (tile_coord, device->mesh->dim);
 
						device->mesh->tiles[tile_id].origin = org;
						device->mesh->tiles[tile_id].tile_group_id = tg->id;
						device->mesh->tiles[tile_id].free = 0;

						hb_mc_coordinate_t coord_val = hb_mc_coordinate((x - org_x), (y - org_y));


						error = hb_mc_tile_set_origin_registers(device->mc, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
						if (error != HB_MC_SUCCESS) { 
							 bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin registers CSR_TGO_X/Y.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
							return HB_MC_FAIL;
						}



						error = hb_mc_tile_set_origin_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_coord_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord_val));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_id_symbol(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord_val), &(tg->dim));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) id symbol __bsg_id.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_tile_group_id_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg->id));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_grid_dim_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg->grid_dim));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) grid size symbol __bsg_grid_size.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
							return HB_MC_FAIL;
						}
					}
				}
		
				tg->origin = hb_mc_coordinate(org_x, org_y);
				tg->status = HB_MC_TILE_GROUP_STATUS_ALLOCATED;


				bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) allocated at origin (%d,%d).\n", 
						_func__,
						tg-> grid_id,
						hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
						hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
						hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));	

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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_tile_group_enqueue (device_t* device, grid_id_t grid_id, hb_mc_coordinate_t tg_id, hb_mc_dimension_t grid_dim, hb_mc_dimension_t dim, char* name, uint32_t argc, uint32_t argv[]) {

	if (device->num_tile_groups == device->tile_group_capacity) { 
		device->tile_group_capacity *= 2;
		device->tile_groups = (tile_group_t *) realloc (device->tile_groups, device->tile_group_capacity * sizeof(tile_group_t));
		if (device->tile_groups == NULL) {
			bsg_pr_err("%s: failed to allocate space for tile_group_t structs.\n", __func__);
			return HB_MC_NOMEM;
		}
	}
	

	tile_group_t* tg = &device->tile_groups[device->num_tile_groups];
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
		bsg_pr_err("%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n", __func__, tg->grid_id, tg->id.x, tg->id.y); 
		return HB_MC_UNINITIALIZED;
	}
	

	tg->kernel = (kernel_t *) malloc (sizeof(kernel_t));
	if (tg->kernel == NULL) { 
		bsg_pr_err("%s: failed to allocated space for kernel_t struct.\n", __func__);
		return HB_MC_NOMEM;
	}
	tg->kernel->name = name;
	tg->kernel->argc = argc;
	tg->kernel->argv = argv;
	tg->kernel->finish_signal_addr = hb_mc_tile_group_get_finish_signal_addr(tg); 
		
	device->num_tile_groups += 1;
	
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) initialized.\n", 
			_func__,
			tg->grid_id,
			hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim),
			hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id)) ;

	return HB_MC_SUCCESS;
}




/**
 * Launches a tile group by sending packets to each tile in the tile group setting the argc, argv, finish_addr and kernel pointer.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if tile group is launched successfully and HB_MC_FAIL otherwise.
 */
int hb_mc_tile_group_launch (device_t *device, tile_group_t *tg) {

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (device->mc); 

	hb_mc_eva_t args_eva;
	error = hb_mc_device_malloc (device, (tg->kernel->argc) * sizeof(uint32_t), &args_eva); /* allocate device memory for arguments */
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to allocate space on device for grid %d tile group (%d,%d) arguments.\n", __func__, tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
		return HB_MC_NOMEM;
	}

	error = hb_mc_device_memcpy(device, reinterpret_cast<void *>(args_eva), (void *) &(tg->kernel->argv[0]), (tg->kernel->argc) * sizeof(uint32_t), hb_mc_memcpy_to_device); /* transfer the arguments to dram */
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to copy grid %d tile group (%d,%d) arguments to device.\n", __func__, tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id)); 
		return HB_MC_FAIL;
	}
	
	hb_mc_eva_t kernel_eva; 
	error = hb_mc_loader_symbol_to_eva (device->program->bin, device->program->bin_size, tg->kernel->name, &kernel_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: invalid kernel name %s for grid %d tile group (%d,%d).\n", __func__, tg->kernel->name, tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
		return HB_MC_FAIL;
	}	

	hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 
	hb_mc_npa_t finish_signal_npa = hb_mc_npa(host_coordinate, tg->kernel->finish_signal_addr); 


	hb_mc_coordinate_t relative_coord; 
	hb_mc_idx_t tile_id;
	for (int y = hb_mc_coordinate_get_y(tg->origin); y < hb_mc_coordinate_get_y(tg->origin) + hb_mc_dimension_get_y(tg->dim); y++){
		for (int x = hb_mc_coordinate_get_x(tg->origin); x < hb_mc_coordinate_get_x(tg->origin) + hb_mc_dimension_get_x(tg->dim); x++){

			relative_coord = hb_mc_get_relative_coordinate (device->mesh->origin, hb_mc_coordinate(x, y));
			tile_id = hb_mc_coordinate_to_flat_index (relative_coord, device->mesh->dim); 




			error = hb_mc_tile_write32(device->mc, &(device->mesh->tiles[tile_id].coord), &ARGC_REG, tg->kernel->argc); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to write argc to tile (%d,%d) for grid %d tile group (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord), tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
				return HB_MC_FAIL;
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) argc to %d.\n", __func__, tile_id, x, y, tg->kernel->argc);


			error = hb_mc_tile_write32(device->mc, &(device->mesh->tiles[tile_id].coord), &ARGV_REG, args_eva); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to write argv pointer to tile (%d,%d) for grid %d tile group (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord), tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
				return HB_MC_FAIL;
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) argv to 0x%x.\n", __func__, tile_id, x, y, args_eva);


			hb_mc_eva_t finish_signal_eva;
			size_t sz; 
			error = hb_mc_npa_to_eva (cfg, tg->map, &(device->mesh->tiles[tile_id].coord), &(finish_signal_npa), &finish_signal_eva, &sz); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to aquire finish signal address eva from npa.\n", __func__); 
				return HB_MC_FAIL;
			}


			error = hb_mc_tile_write32(device->mc, &(device->mesh->tiles[tile_id].coord), &SIGNAL_REG, finish_signal_eva); 
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write finish signal address to tile (%d,%d) for grid %d tile group (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord), tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
				return HB_MC_FAIL;
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) SIGNAL_REG to 0x%x.\n", __func__, tile_id, x, y, finish_signal_host_eva);



			error = hb_mc_tile_write32(device->mc, &(device->mesh->tiles[tile_id].coord), &KERNEL_REG, kernel_eva); 
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write kernel pointer to tile (%d,%d) for grid %d tile group (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord), tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
				return HB_MC_FAIL;
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) KERNEL_REG to 0x%x.\n", __func__, tile_id, x, y, kernel_eva); 
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
 * @return HB_MC_SUCCESS if tile group is launched successfully and HB_MC_FAIL otherwise.
 */
int hb_mc_tile_group_deallocate_tiles(device_t *device, tile_group_t *tg) {
	hb_mc_coordinate_t relative_coord; 
	hb_mc_idx_t tile_id; 
	for (int x = hb_mc_coordinate_get_x(tg->origin); x < hb_mc_coordinate_get_x(tg->origin) + hb_mc_dimension_get_x(tg->dim); x++){
		for (int y = hb_mc_coordinate_get_y(tg->origin); y < hb_mc_coordinate_get_y(tg->origin) + hb_mc_dimension_get_y(tg->dim); y++){
			relative_coord = hb_mc_get_relative_coordinate (device->mesh->origin, hb_mc_coordinate (x, y)); 
			tile_id = hb_mc_coordinate_to_flat_index (relative_coord, device->mesh->dim); 
			
			device->mesh->tiles[tile_id].origin = device->mesh->origin;
			device->mesh->tiles[tile_id].tile_group_id = hb_mc_coordinate( 0, 0);
			device->mesh->tiles[tile_id].free = 1;
		}
	}
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) de-allocated at origin (%d,%d).\n",
			__func__,
			tg->grid_id,
			hb_mc_dimension_get_x(tg->dim), hb_mc_dimension_get_y(tg->dim.y),
			hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id),
			hb_mc_coordinate_get_x(tg->origin), hb_mc_coordinate_get_y(tg->origin));
	
	tg->status = HB_MC_TILE_GROUP_STATUS_FINISHED;

	return HB_MC_SUCCESS;
}




/**
 * Initializes the manycore struct, and a mesh structure inside device struct with list of tiles and their coordinates 
 * @param[in]  device        Pointer to device
 * @param[in]  name          Device name
 * @param[in]  id            Device id
 * @param[in]  dim           Tile pool (mesh) dimensions
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_init (device_t *device, char *name, hb_mc_manycore_id_t id, hb_mc_dimension_t dim) {

	device->mc = (hb_mc_manycore_t*) malloc (sizeof (hb_mc_manycore_t));
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
	device->tile_groups = (tile_group_t *) malloc (device->tile_group_capacity * sizeof(tile_group_t));
	if (device->tile_groups == NULL) {
		bsg_pr_err("%s: failed to allocated space for list of tile groups.\n", __func__);
		return HB_MC_NOMEM;
	}
	memset (device->tile_groups, 0, device->tile_group_capacity * sizeof(tile_group_t));
	device->num_tile_groups = 0;
	device->num_grids = 0;

	return HB_MC_SUCCESS;
}




/**
 * Freezes tiles, loads program binary into all tiles and into dram, and set the symbols and registers for each tile.
 * @param[in]  device        Pointer to device
 * @parma[in]  bin_name      Name of binary elf file
 * @param[in]  id            Id of program's meomry allocator
 * @param[in]  alloc_name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_program_init (device_t *device, char *bin_name, const char *alloc_name, hb_mc_allocator_id_t id) {
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

	uint32_t num_tiles = hb_mc_dimension_to_size (device->mesh->dim);

	// Freeze all tiles 
	for (int tile_id = 0; tile_id < num_tiles; tile_id++) { /* initialize tiles */
		error = hb_mc_tile_freeze(device->mc, &(device->mesh->tiles[tile_id].coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to freeze tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}
	}



	// Load binary into all tiles 
	error = hb_mc_loader_read_program_file (device->program->bin_name, &(device->program->bin), &(device->program->bin_size));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err ("%s: failed to read binary file.\n", __func__); 
		return HB_MC_FAIL;
	}
	

	hb_mc_coordinate_t *tile_list = (hb_mc_coordinate_t *) malloc (num_tiles * sizeof (hb_mc_coordinate_t)); 
	for (int tile_id = 0; tile_id < num_tiles; tile_id ++) {
		tile_list[tile_id] = device->mesh->tiles[tile_id].coord;
	}


	error = hb_mc_loader_load (device->program->bin, device->program->bin_size, device->mc, &default_map, tile_list, num_tiles); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err ("%s: failed to load binary into tiles.\n", __func__); 
		return HB_MC_FAIL;
	}	

	free (tile_list); 
	

	// Initialize program's memory allocator
	error = hb_mc_program_allocator_init (device->program, alloc_name, id); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to initialize memory allocator for program %s.\n", __func__, device->program->bin_name); 
		return HB_MC_UNINITIALIZED;
	}


	for (int tile_id = 0; tile_id < num_tiles; tile_id++) { /* initialize tiles */
	
		hb_mc_coordinate_t coord = hb_mc_get_relative_coordinate (device->mesh->tiles[tile_id].origin, device->mesh->tiles[tile_id].coord); 
		hb_mc_coordinate_t tg_id = hb_mc_coordinate(0, 0);
		hb_mc_dimension_t tg_dim = hb_mc_dimension(1, 1);


		error = hb_mc_tile_set_origin_registers(device->mc, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin registers CSR_TGO_X/Y.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_origin_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_coord_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_id_symbol(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord), &(device->mesh->dim));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) id symbol __bsg_id.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_tile_group_id_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg_id));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL; 
		}


		error = hb_mc_tile_set_grid_dim_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg_dim));	
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d, %d) grid size symbol __bsg_grid_size.\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}
	}


 	
	// unfreeze all tiles
	for (int tile_id = 0; tile_id < num_tiles; tile_id ++) {
		error = hb_mc_tile_write32(device->mc, &(device->mesh->tiles[tile_id].coord), &KERNEL_REG, 0x1); /* Initilize the kernel register in tile*/  
		if (error != HB_MC_SUCCESS) {
			bsg_pr_err("%s: failed to initialize kernel register to 0x1 in tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}

		error = hb_mc_tile_unfreeze(device->mc, &(device->mesh->tiles[tile_id].coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to unfreeze tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(device->mesh->tiles[tile_id].coord), hb_mc_coordinate_get_y(device->mesh->tiles[tile_id].coord));
			return HB_MC_FAIL;
		}
	}
	return HB_MC_SUCCESS;
}





/**
 * Initializes program's memory allocator and creates a memory manager
 * @param[in]  program       Pointer to program
 * @param[in]  id            Id of program's meomry allocator
 * @param[in]  name    Unique name of program's memory allocator
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_program_allocator_init (hb_mc_program_t *program, const char *name, hb_mc_allocator_id_t id) {
	int error;

	program->allocator = (hb_mc_allocator_t *) malloc (sizeof (hb_mc_allocator_t)); 
	if (program->allocator == NULL) {
		bsg_pr_err("%s: failed to allcoat space on host for program's hb_mc_allocator_t struct.\n", __func__);
		return HB_MC_FAIL;
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
		bsg_pr_err("%s: failed to aquire _bsg_dram_end_addr eva from binary file.\n", __func__); 
		return HB_MC_INVALID;
	}

	uint32_t alignment = 32;
	uint32_t start = program_end_eva + alignment - (program_end_eva % alignment); /* start at the next aligned block */
	uint32_t size = DRAM_SIZE;
	program->allocator->memory_manager = (awsbwhal::MemoryManager *) new awsbwhal::MemoryManager(DRAM_SIZE, start, alignment); 

	return HB_MC_SUCCESS;	
}



	

/**
 * Checks to see if all tile groups in a device are finished.
 * @param[in]  device        Pointer to device
 * returns HB_MC_SUCCESS if all tile groups are finished, and HB_MC_FAIL otherwise.
 */
int hb_mc_device_all_tile_groups_finished(device_t *device) {
	
	tile_group_t *tg = device->tile_groups; 
	for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) {
		if (tg->status != HB_MC_TILE_GROUP_STATUS_FINISHED )
			return HB_MC_FAIL; 
	}

	return HB_MC_SUCCESS;
}




/**
 * Waits for a tile group to send a finish packet to device.
 * @param[in]  device        Pointer to device
 * return HB_MC_SUCCESS after a tile group is finished, gets stuck in infinite look if no tile group finishes.
 */
int hb_mc_device_wait_for_tile_group_finish_any(device_t *device) {
	int error; 

	int tile_group_finished = 0;
	hb_mc_request_packet_t recv, finish;
	hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 

	while (!tile_group_finished) {

		error = hb_mc_manycore_request_rx (device->mc, &recv, -1); 
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to read fifo for finish packet from device.\n", __func__);
			return HB_MC_FAIL;
		}

		/* Check all tile groups to see if the received packet is the finish packet from one of them */
		tile_group_t *tg = device->tile_groups;
		for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) {
			if (tg->status == HB_MC_TILE_GROUP_STATUS_LAUNCHED) {

				hb_mc_request_packet_set_x_dst(&finish, (uint8_t) hb_mc_coordinate_get_x(host_coordinate));
				hb_mc_request_packet_set_y_dst(&finish, (uint8_t) hb_mc_coordinate_get_y(host_coordinate));
				hb_mc_request_packet_set_x_src(&finish, (uint8_t) hb_mc_coordinate_get_x(tg->origin));
				hb_mc_request_packet_set_y_src(&finish, (uint8_t) hb_mc_coordinate_get_y(tg->origin));
				hb_mc_request_packet_set_data(&finish, 0x1 /* TODO: Hardcoded */);
				hb_mc_request_packet_set_mask(&finish, HB_MC_PACKET_REQUEST_MASK_WORD);
				hb_mc_request_packet_set_op(&finish, HB_MC_PACKET_OP_REMOTE_STORE);
				hb_mc_request_packet_set_addr(&finish, tg->kernel->finish_signal_addr >> 2);

				if (hb_mc_request_packet_equals(&recv, &finish) == HB_MC_SUCCESS) {
		
					bsg_pr_dbg("%s: Finish packet received for grid %d tile group (%d,%d): src (%d,%d), dst (%d,%d), addr: 0x%x, data: %d.\n", 
							__func__, 
							tg->grid_id, 
							hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id), 
							recv.x_src, recv.y_src, 
							recv.x_dst, recv.y_dst, 
							recv.addr, recv.data);

					error = hb_mc_tile_group_deallocate_tiles(device, tg);
					if (error != HB_MC_SUCCESS) { 
						bsg_pr_err("%s: failed to deallocate grid %d tile group (%d,%d).\n", __func__, tg->grid_id, hb_mc_coordinate_get_x(tg->id), hb_mc_coordinate_get_y(tg->id));
						return HB_MC_FAIL;
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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_tile_groups_execute (device_t *device) {

	int error ;
	/* loop untill all tile groups have been allocated, launched and finished. */
	while(hb_mc_device_all_tile_groups_finished(device) != HB_MC_SUCCESS) {
		/* loop over all tile groups and try to launch as many as possible */
		tile_group_t *tg = device->tile_groups;
		for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) { 
			if (tg->status == HB_MC_TILE_GROUP_STATUS_INITIALIZED) {
				error = hb_mc_tile_group_allocate_tiles(device, tg) ;
				if (error == HB_MC_SUCCESS) {
					error = hb_mc_tile_group_launch(device, tg);
					if (error != HB_MC_SUCCESS) {
						bsg_pr_err("%s: failed to launch tile group %d.\n", __func__, tg_num);
						return HB_MC_FAIL;
					}
				}
			}
		}

		/* wait for a tile group to finish */
		error = hb_mc_device_wait_for_tile_group_finish_any(device);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: tile group not finished, something went wrong.\n", __func__); 
			return HB_MC_FAIL;
		}

	}

	return HB_MC_SUCCESS;
}




/**
 * Deletes memory manager, device and manycore struct, and freezes all tiles in device.
 * @param[in]  device        Pointer to device
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_finish (device_t *device) {

	int error;

	for (int tile_id = 0; tile_id < hb_mc_dimension_to_size (device->mesh->dim); tile_id ++) {
		error = hb_mc_tile_freeze(device->mc, &(device->mesh->tiles[tile_id].coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to freeze tile.\n", __func__); 
			return HB_MC_FAIL;
		}
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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_malloc (device_t *device, uint32_t size, hb_mc_eva_t *eva) {
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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_free (device_t *device, eva_t eva) {


	if (!device->program->allocator->memory_manager) {
		bsg_pr_err("%s: memory manager not initialized.\n", __func__);
		return HB_MC_FAIL; 
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
 * @param[in]  hb_mc_memcpy_kind         Direction of copy (hb_mc_memcpy_to_device / hb_mc_memcpy_to_host)
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_memcpy (device_t *device, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind) {

	int error;
	
	hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 
	size_t sz = count / sizeof(uint8_t); 

	if (kind == hb_mc_memcpy_to_device) {
		hb_mc_eva_t dst_eva = (hb_mc_eva_t) reinterpret_cast<uintptr_t>(dst);

		error =  hb_mc_manycore_eva_write(device->mc, &default_map, &host_coordinate, &dst_eva, src, sz);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to copy memory to device.\n", __func__);
			return HB_MC_FAIL;
		}
	}
	
	else if (kind == hb_mc_memcpy_to_host) { 
		hb_mc_eva_t src_eva = (hb_mc_eva_t) reinterpret_cast<uintptr_t>(src);

		error = hb_mc_manycore_eva_read(device->mc, &default_map, &host_coordinate, &src_eva, dst, sz); 
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to copy memory from device.\n", __func__);
			return HB_MC_FAIL;
		}

	}

	else {
		bsg_pr_err("%s: invalid copy type. Copy type can be one of hb_mc_memcpy_to_device or hb_mc_memcpy_to_host.\n", __func__);
		return HB_MC_FAIL; 
	}

	return HB_MC_SUCCESS;
}




/**
 * Calculates and returns a tile group's finish signal address based on tile group id and grid dimension
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group 
 * @return     finish_signal_addr
 */
static hb_mc_epa_t hb_mc_tile_group_get_finish_signal_addr(tile_group_t *tg) { 
	hb_mc_epa_t finish_addr = FINISH_BASE_ADDR + ((hb_mc_coordinate_get_y(tg->id) * hb_mc_dimension_get_x(tg->grid_dim) + hb_mc_coordinate_get_x(tg->id)) << 2); /* TODO: Hardcoded */
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
 * @return     flat_idx
 */
static hb_mc_idx_t hb_mc_coordinate_to_flat_index (hb_mc_coordinate_t coord, hb_mc_dimension_t dim) {
	hb_mc_idx_t flat_idx = hb_mc_coordinate_get_y(coord) * hb_mc_dimension_get_x(dim) + hb_mc_coordinate_get_x(coord); 
	return flat_idx;
} 




/**
 * Calculates and returns a 1D size based on 2D dimensions 
 * @parma[in]  dim           Dimensions 
 * @return     1D flat size
 */
static int  hb_mc_dimension_to_size (hb_mc_dimension_t dim) { 
	return (hb_mc_dimension_get_x(dim) * hb_mc_dimension_get_y(dim)); 
} 

