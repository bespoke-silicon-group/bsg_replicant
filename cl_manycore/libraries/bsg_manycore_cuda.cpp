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

static const uint32_t KERNEL_REG = 0x1000 >> 2; //!< EPA of kernel. 
static const uint32_t ARGC_REG = 0x1004 >> 2; //!< EPA of number of arguments kernel will use. 
static const uint32_t ARGV_REG = 0x1008 >> 2; //!< EPA of arguments for kernel. 
static const uint32_t SIGNAL_REG = 0x100c >> 2; //!< EPA of register that holds signal address. Tile will write to this address once it completes the kernel.   


static const hb_mc_epa_t FINISH_BASE_ADDR = 0xF000; //!< EPA to which tile group sends a finish packet once it finishes executing a kernel  

static awsbwhal::MemoryManager *mem_manager[1] = {(awsbwhal::MemoryManager *) 0}; /* This array has an element for every EVA <-> NPA mapping. Currently, only one mapping is supported. */

static uint32_t const DRAM_SIZE = 0x80000000;




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
		x_list[i] = tiles[i].coord.x;
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
		y_list[i] = tiles[i].coord.y;
	}
	return HB_MC_SUCCESS;
}




/*!
 * creates a awsbwhal::MemoryManager object and stores it in a global table.
 * @param eva_id which specifies which EVA <-> NPA mapping.
 * @param bin_name path to ELF binary
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
static int hb_mc_create_memory_manager (eva_id_t eva_id, char *bin_name) {
	eva_t program_end;
	if (symbol_to_eva(bin_name, "_bsg_dram_end_addr", &program_end) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	uint32_t alignment = 32;
	uint32_t start = program_end + alignment - (program_end % alignment); /* start at the next aligned block */
	uint32_t size = DRAM_SIZE;
	mem_manager[eva_id] = new awsbwhal::MemoryManager(DRAM_SIZE, start, alignment); 
	return HB_MC_SUCCESS;	
}

static int hb_mc_write_tile_reg(uint8_t fd, eva_t eva_id, tile_t *tile, uint32_t epa, uint32_t val) {
	int error =  hb_mc_copy_to_epa (fd, tile->coord.x, tile->coord.y, epa, &val, 1);
	if (error != HB_MC_SUCCESS)
		return HB_MC_FAIL; /* could not memcpy */
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

	
	if (dim.x <= 0){
		bsg_pr_err("%s: Mesh X dimension (%d) not valid.\n", __func__, dim.x); 
		return HB_MC_FAIL;
	}
	if (dim.y <= 0){
		bsg_pr_err("%s: Mesh Y dimension (%d) not valid.\n", __func__, dim.y);
		return HB_MC_FAIL;
	}
	if (dim.x > device_dim.x){
		bsg_pr_err("%s: Mesh X dimension (%d) larger than device X dimension.\n", __func__, dim.x, device_dim.x); 
		return HB_MC_FAIL;
	}
	if (dim.y > device_dim.y){
		bsg_pr_err("%s: Mesh Y dimension (%d) larger than device Y dimension.\n", __func__, dim.y, device_dim.y);
		return HB_MC_FAIL;
	}

	tile_t* tiles = new tile_t [dim.x * dim.y];
	for (int x = default_origin.x; x < default_origin.x + dim.x; x++){
		for (int y = default_origin.y; y < default_origin.y + dim.y; y++){
			int tile_id = (y - default_origin.y) * dim.x + (x - default_origin.x);
			tiles[tile_id].coord.x = x;
			tiles[tile_id].coord.y = y;
			tiles[tile_id].origin.x = default_origin.x;
			tiles[tile_id].origin.y = default_origin.y;
			tiles[tile_id].tile_group_id.x = -1;
			tiles[tile_id].tile_group_id.y = -1;
			tiles[tile_id].free = 1;
		}
	}

	mesh_t *mesh = new mesh_t; 
	mesh->dim.x = dim.x;
	mesh->dim.y = dim.y;
	mesh->origin.x = default_origin.x;
	mesh->origin.y = default_origin.y;
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
	if (hb_mc_fifo_check(device->fd) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to verify device.\n", __func__); 
		return HB_MC_FAIL;
	}

	int error; 
	for (hb_mc_idx_t tg_id_x = 0; tg_id_x < grid_dim.x; tg_id_x ++) { 
		for (hb_mc_idx_t tg_id_y = 0; tg_id_y < grid_dim.y; tg_id_y ++) { 
			hb_mc_coordinate_t tg_id = {.x = tg_id_x, .y = tg_id_y};
			error = hb_mc_tile_group_enqueue(device, device->num_grids, tg_id, grid_dim, tg_dim, name, argc, argv); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to initialize tile group (%d,%d) of grid %d.\n", __func__, tg_id_x, tg_id_y, device->num_grids);
				return HB_MC_FAIL;
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
	if (tg->dim.x > device->mesh->dim.x){
		bsg_pr_err("%s: tile group X dimension (%d) larger than mesh X dimension (%d).\n", __func__, tg->dim.x, device->mesh->dim.x);
		return HB_MC_FAIL;
	}
	if (tg->dim.y > device->mesh->dim.y){
		bsg_pr_err("%s: tile group Y dimension (%d) larger than mesh Y dimension (%d).\n", __func__, tg->dim.y, device->mesh->dim.y);
		return HB_MC_FAIL;
	}
	for (hb_mc_idx_t org_y = device->mesh->origin.y; org_y <= (device->mesh->origin.y + device->mesh->dim.y - tg->dim.y); org_y++){
		for (hb_mc_idx_t org_x = device->mesh->origin.x; org_x <= (device->mesh->origin.x + device->mesh->dim.x - tg->dim.x); org_x++){
			int free = 1;
			hb_mc_idx_t tile_id;
			for (hb_mc_idx_t x = org_x; x < org_x + tg->dim.x; x++){
				for (hb_mc_idx_t y = org_y; y < org_y + tg->dim.y; y++){
					tile_id = (y - device->mesh->origin.y) * device->mesh->dim.x + (x - device->mesh->origin.x);
					free = free & device->mesh->tiles[tile_id].free;
				}
			}
			if (free){

				hb_mc_coordinate_t org = { .x = org_x, .y = org_y }; 
				error = hb_mc_origin_eva_map_init (tg->map, org); 
				if (error != HB_MC_SUCCESS) { 
					bsg_pr_err("%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n", __func__, tg->grid_id, tg->id.x, tg->id.y);
					return HB_MC_FAIL;
				}

				for (hb_mc_idx_t x = org_x; x < org_x + tg->dim.x; x++){
					for (hb_mc_idx_t y = org_y; y < org_y + tg->dim.y; y++){
						tile_id = (y - device->mesh->origin.y) * device->mesh->dim.x + (x - device->mesh->origin.x);
						device->mesh->tiles[tile_id].origin = org;
						device->mesh->tiles[tile_id].tile_group_id = tg->id;
						device->mesh->tiles[tile_id].free = 0;

						hb_mc_coordinate_t coord_val = {.x = (x - org_x), .y = (y - org_y)};


						error = hb_mc_tile_set_origin_registers(device->mc, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
						if (error != HB_MC_SUCCESS) { 
							 bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin registers CSR_TGO_X/Y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}



						error = hb_mc_tile_set_origin_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_coord_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord_val));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n", __func__, device->mesh->tiles[tile_id].coord);
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_id_symbol(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord_val), &(tg->dim));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) id symbol __bsg_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_tile_group_id_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg->id));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}


						error = hb_mc_tile_set_grid_dim_symbols(device->mc, tg->map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg->grid_dim));
						if (error != HB_MC_SUCCESS) { 
							bsg_pr_err("%s: failed to set tile (%d,%d) grid size symbol __bsg_grid_size.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}




///////// DEPRECATED
/*
						if (hb_mc_tile_set_origin_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, device->mesh->tiles[tile_id].origin) != HB_MC_SUCCESS){
							bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}


						if (hb_mc_tile_set_coord_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, coord_val) != HB_MC_SUCCESS){
							bsg_pr_err("%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n", __func__, device->mesh->tiles[tile_id].coord);
							return HB_MC_FAIL;
						}


						if (hb_mc_tile_set_id_symbol_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, coord_val, tg->dim) != HB_MC_SUCCESS){
							bsg_pr_err("%s: failed to set tile (%d,%d) id symbol __bsg_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}


						if (hb_mc_tile_set_tile_group_id_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, tg->id) != HB_MC_SUCCESS){
							bsg_pr_err("%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}


						if (hb_mc_tile_set_grid_dim_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, tg->grid_dim) != HB_MC_SUCCESS){
							bsg_pr_err("%s: failed to set tile (%d,%d) grid size symbol __bsg_grid_size.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
							return HB_MC_FAIL;
						}
*/


					}
				}
		
				tg->origin.x = org_x;
				tg->origin.y = org_y;
				tg->status = HB_MC_TILE_GROUP_STATUS_ALLOCATED;


				bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) allocated at origin (%d,%d).\n", __func__, tg-> grid_id, tg->dim.x, tg->dim.y, tg->id.x, tg->id.y, tg->origin.x, tg->origin.y);	
				return HB_MC_SUCCESS;
			}
		}
	}
	return HB_MC_FAIL;
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
			return HB_MC_FAIL;
		}
	}
	

	tile_group_t* tg = (tile_group_t *)(device->tile_groups + device->num_tile_groups) ; 
	tg->dim = dim;
	tg->origin = device->mesh->origin;
	tg->id = tg_id;
	tg->grid_id = grid_id;
	tg->grid_dim = grid_dim;
	tg->status = HB_MC_TILE_GROUP_STATUS_INITIALIZED;

	tg->map = (hb_mc_eva_map_t *) malloc (sizeof(hb_mc_eva_map_t)); 
	if (tg->map == NULL) { 
		bsg_pr_err ("%s: failed to allocate space for tile group hb_mc_eva_map_t struct map.\n", __func__);
		return HB_MC_FAIL;
	}
	int error = hb_mc_origin_eva_map_init (tg->map, tg->origin); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to initialize grid %d tile group (%d,%d) eva map origin.\n", __func__, tg->grid_id, tg->id.x, tg->id.y); 
		return HB_MC_FAIL;
	}
	

	tg->kernel = (kernel_t *) malloc (sizeof(kernel_t));
	if (tg->kernel == NULL) { 
		bsg_pr_err("%s: failed to allocated space for kernel_t struct.\n", __func__);
		return HB_MC_FAIL;
	}
	tg->kernel->name = name;
	tg->kernel->argc = argc;
	tg->kernel->argv = argv;
	tg->kernel->finish_signal_addr = FINISH_BASE_ADDR + ((tg->id.y * tg->grid_dim.x + tg->id.x) << 2); /* TODO: Hardcoded */
		
	device->num_tile_groups += 1;
	
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) initialized.\n", __func__, tg->grid_id, tg->dim.x, tg->dim.y, tg->id.x, tg->id.y) ;

	return HB_MC_SUCCESS;
}




/**
 * Launches a tile group by sending packets to each tile in the tile group setting the argc, argv, finish_addr and kernel pointer.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if tile group is launched successfully and HB_MC_FAIL otherwise.
 */
int hb_mc_tile_group_launch (device_t *device, tile_group_t *tg) {

	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (device->mc); 

	hb_mc_eva_t args_eva;
	int error = hb_mc_device_malloc (device, (tg->kernel->argc) * sizeof(uint32_t), &args_eva); /* allocate device memory for arguments */
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to allocate space on device for grid %d tile group (%d,%d) arguments.\n", __func__, tg->grid_id, tg->id.x, tg->id.y);
		return HB_MC_FAIL;
	}

	error = hb_mc_device_memcpy(device, reinterpret_cast<void *>(args_eva), (void *) &(tg->kernel->argv[0]), (tg->kernel->argc) * sizeof(uint32_t), hb_mc_memcpy_to_device); /* transfer the arguments to dram */
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to copy grid %d tile group (%d,%d) arguments to device.\n", __func__, tg->grid_id, tg->id.x, tg->id.y); 
		return HB_MC_FAIL;
	}
	
	hb_mc_eva_t kernel_eva; 
	error = hb_mc_loader_symbol_to_eva (device->program->bin, device->program->bin_size, tg->kernel->name, &kernel_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: invalid kernel name %s for grid %d tile group (%d,%d).\n", __func__, tg->kernel->name, tg->grid_id, tg->id.x, tg->id.y);
		return HB_MC_FAIL;
	}	

///// DEPRECATED
/*
	error = symbol_to_eva(device->program->bin_name, tg->kernel->name, &kernel_eva); 
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: invalid kernel name %s for grid %d tile group (%d,%d).\n", __func__, tg->kernel->name, tg->grid_id, tg->id.x, tg->id.y); 
		return HB_MC_FAIL;
	}
*/



	hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(device->mc); 
	hb_mc_npa_t finish_signal_npa = hb_mc_npa(host_coordinate, tg->kernel->finish_signal_addr); 


	int tile_id;
	for (int y = tg->origin.y; y < tg->origin.y + tg->dim.y; y++){
		for (int x = tg->origin.x; x < tg->origin.x + tg->dim.x; x++){
			tile_id = (y - device->mesh->origin.y) * device->mesh->dim.x + (x - device->mesh->origin.x);


			error = hb_mc_write_tile_reg(device->fd, device->eva_id, &(device->mesh->tiles[tile_id]), ARGC_REG, tg->kernel->argc); /* write argc to tile */
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write argc %d to device for grid %d tile group (%d,%d).\n", __func__, tg->kernel->argc, tg->grid_id, tg->id.x, tg->id.y); 
				return HB_MC_FAIL; 
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) argc to %d.\n", __func__, tile_id, x, y, tg->kernel->argc);

			error = hb_mc_write_tile_reg(device->fd, device->eva_id, &(device->mesh->tiles[tile_id]), ARGV_REG, args_eva); /* write EVA of arguments to tile group */
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write argv to device for grid %d tile group (%d,%d).\n", __func__, tg->grid_id, tg->id.x, tg->id.y);
				return HB_MC_FAIL; 
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) argv to 0x%x.\n", __func__, tile_id, x, y, args_eva);

			uint32_t host_coord_x, host_coord_y;
			error = hb_mc_get_config(device->fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &host_coord_x);
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to get device-host interface X coordiante.\n", __func__);
				return HB_MC_FAIL;
			}

			error = hb_mc_get_config(device->fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &host_coord_y);
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to get device-host interface Y coordiante.\n", __func__);
				return HB_MC_FAIL;
			}




			hb_mc_eva_t finish_signal_eva;
			size_t sz; 
			error = hb_mc_npa_to_eva (cfg, tg->map, &(device->mesh->tiles[tile_id].coord), &(finish_signal_npa), &finish_signal_eva, &sz); 
			if (error != HB_MC_SUCCESS) { 
				bsg_pr_err("%s: failed to aquire finish signal address eva from npa.\n", __func__); 
				return HB_MC_FAIL;
			}

			error = hb_mc_write_tile_reg(device->fd, device->eva_id, &(device->mesh->tiles[tile_id]), SIGNAL_REG, finish_signal_eva); 
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write finish_signal_addr eva 0x%x to device for grid %d tile group (%d,%d).\n", finish_signal_eva, tg->grid_id, tg->id.x, tg->id.y);
				return HB_MC_FAIL;
			}




//////// DEPRECATED
/*
			npa_t finish_signal_host_npa = {host_coord_x, host_coord_y, tg->kernel->finish_signal_addr};
			bsg_pr_dbg("%s: Finish signal <X, Y, EPA> is: <%d, %d, 0x%x>.\n", __func__, host_coord_x, host_coord_y, tg->kernel->finish_signal_addr) ;
		
			eva_t finish_signal_host_eva;
			error = hb_mc_npa_to_eva_deprecated(device->eva_id, &finish_signal_host_npa, &finish_signal_host_eva); // tile will write to this address when it finishes executing the kernel
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to get finish_signal_host_eva from finish_signal_host_npa.\n", __func__);
				return HB_MC_FAIL;
			}
			
			error = hb_mc_write_tile_reg(device->fd, device->eva_id, &(device->mesh->tiles[tile_id]), SIGNAL_REG, finish_signal_host_eva); 
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write finish_signal_addr %d to device for grid %d tile group (%d,%d).\n", __func__, finish_signal_host_eva, tg->grid_id, tg->id.x, tg->id.y);
				return HB_MC_FAIL;
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) SIGNAL_REG to 0x%x.\n", __func__, tile_id, x, y, finish_signal_host_eva);
*/



			error = hb_mc_write_tile_reg(device->fd, device->eva_id, &(device->mesh->tiles[tile_id]), KERNEL_REG, kernel_eva); /* write kernel EVA to tile group */
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write kernel eva %d to device for grid %d tile group (%d,%d).\n", __func__, kernel_eva, tg->grid_id, tg->id.x, tg->id.y);
				return HB_MC_FAIL; 
			}
			bsg_pr_dbg("%s: Setting tile[%d] (%d,%d) KERNEL_REG to 0x%x.\n", __func__, tile_id, x, y, kernel_eva); 
		}
	} 

	tg->status=HB_MC_TILE_GROUP_STATUS_LAUNCHED;
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) launched at origin (%d,%d).\n", __func__, tg->grid_id, tg->dim.x, tg->dim.y, tg->id.x, tg->id.y, tg->origin.x, tg->origin.y);
	return HB_MC_SUCCESS;
}




/**
 * De-allocates all tiles in tile group, and resets their tile-group id and origin in the device book keeping.
 * @param[in]  device        Pointer to device
 * @parma[in]  tg            Pointer to tile group
 * @return HB_MC_SUCCESS if tile group is launched successfully and HB_MC_FAIL otherwise.
 */
int hb_mc_tile_group_deallocate_tiles(device_t *device, tile_group_t *tg) {
	if (hb_mc_fifo_check(device->fd) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to verify device.\n", __func__); 
		return HB_MC_FAIL;
	}

	int tile_id;
	for (int x = tg->origin.x; x < tg->origin.x + tg->dim.x; x++){
		for (int y = tg->origin.y; y < tg->origin.y + tg->dim.y; y++){
			tile_id = (y - device->mesh->origin.y) * device->mesh->dim.x + (x - device->mesh->origin.x);
			device->mesh->tiles[tile_id].origin = device->mesh->origin;
			device->mesh->tiles[tile_id].tile_group_id = {.x = 0, .y = 0};
			device->mesh->tiles[tile_id].free = 1;
		}
	}
	bsg_pr_dbg("%s: Grid %d: %dx%d tile group (%d,%d) de-allocated at origin (%d,%d).\n", __func__, tg->grid_id, tg->dim.x, tg->dim.y, tg->id.x, tg->id.y, tg->origin.x, tg->origin.y);
	
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

	hb_mc_fifo_init(&(device->fd));

	device->mc = (hb_mc_manycore_t*) malloc (sizeof (hb_mc_manycore_t));
	*(device->mc) = {0};
	
	int error = hb_mc_manycore_init(device->mc, name, id); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to initialize manycore.\n", __func__);
		return HB_MC_FAIL;
	} 
	
	device->eva_id = 0;  // To be deprecated soon


	error = hb_mc_mesh_init(device, dim);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to initialize mesh.\n", __func__);
		return HB_MC_FAIL;
	}


	device->tile_group_capacity = 1;
	device->tile_groups = (tile_group_t *) malloc (device->tile_group_capacity * sizeof(tile_group_t));
	if (device->tile_groups == NULL) {
		bsg_pr_err("%s: failed to allocated space for list of tile groups.\n", __func__);
		return HB_MC_FAIL;
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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_program_init (device_t *device, char *bin_name) {
	int error;
	
	device->program = (hb_mc_program_t *) malloc (sizeof (hb_mc_program_t));
	if (device->program == NULL) { 
		bsg_pr_err("%s: failed to allocate space on host for device hb_mc_program_t struct.\n", __func__);
		return HB_MC_FAIL;
	}

	device->program->bin_name = strdup (bin_name);

	uint32_t num_tiles = device->mesh->dim.x * device->mesh->dim.y; 	


	// Freeze all tiles 
	for (int tile_id = 0; tile_id < num_tiles; tile_id++) { /* initialize tiles */
		error = hb_mc_tile_freeze(device->mc, &(device->mesh->tiles[tile_id].coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to freeze tile (%d,%d).\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
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
	


/////// DEPRECATED
/*
	uint8_t x_list[num_tiles], y_list[num_tiles];	
	hb_mc_get_x(device->mesh->tiles, &x_list[0], num_tiles);
	hb_mc_get_y(device->mesh->tiles, &y_list[0], num_tiles); 
	hb_mc_load_binary(device->fd, device->program->bin_name, &x_list[0], &y_list[0], num_tiles);
*/



	for (int tile_id = 0; tile_id < num_tiles; tile_id++) { /* initialize tiles */
	
		hb_mc_coordinate_t coord = { .x = device->mesh->tiles[tile_id].coord.x - device->mesh->tiles[tile_id].origin.x, .y = device->mesh->tiles[tile_id].coord.y - device->mesh->tiles[tile_id].origin.y};
		hb_mc_coordinate_t tg_id = {.x = 0, .y = 0};
		hb_mc_dimension_t tg_dim = {.x = 1, .y = 1};


		error = hb_mc_tile_set_origin_registers(device->mc, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin registers CSR_TGO_X/Y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_origin_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(device->mesh->tiles[tile_id].origin));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_coord_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_id_symbol(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(coord), &(device->mesh->dim));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) id symbol __bsg_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_tile_group_id_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg_id));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL; 
		}


		error = hb_mc_tile_set_grid_dim_symbols(device->mc, &default_map, device->program->bin, device->program->bin_size, &(device->mesh->tiles[tile_id].coord), &(tg_dim));	
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d, %d) grid size symbol __bsg_grid_size.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


///////// DEPRECATED 
/*
		error = hb_mc_tile_set_origin_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, device->mesh->tiles[tile_id].origin);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group origin symbols __bsg_grp_org_x/y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_coord_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, coord);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) coordinate symbols __bsg_x/y.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_id_symbol_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, coord, device->mesh->dim);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) id symbol __bsg_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}


		error = hb_mc_tile_set_tile_group_id_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, tg_id);
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d,%d) tile group id symbold __bsg_tile_group_id.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL; 
		}


		error = hb_mc_tile_set_grid_dim_symbols_dep(device->fd, device->eva_id, device->program->bin_name, device->mesh->tiles[tile_id].coord, tg_dim);	
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to set tile (%d, %d) grid size symbol __bsg_grid_size.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}
*/

	}


 	
	// unfreeze all tiles
	for (int tile_id = 0; tile_id < num_tiles; tile_id ++) {
		error = hb_mc_write_tile_reg(device->fd, device->eva_id, &(device->mesh->tiles[tile_id]), KERNEL_REG, 0x1); /* initialize the kernel register */
		if (error != HB_MC_SUCCESS) {
			bsg_pr_err("%s: failed to write 01 to tile (%d,%d) KERNEL_REG.\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}

		error = hb_mc_tile_freeze(device->mc, &(device->mesh->tiles[tile_id].coord));
		if (error != HB_MC_SUCCESS) { 
			bsg_pr_err("%s: failed to freeze tile (%d,%d).\n", __func__, device->mesh->tiles[tile_id].coord.x, device->mesh->tiles[tile_id].coord.y);
			return HB_MC_FAIL;
		}
	}

	// create a memory manager object 
	if (hb_mc_create_memory_manager(device->eva_id, device->program->bin_name) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to initialize device memory manager.\n", __func__); 
		return HB_MC_FAIL;
	}

	return HB_MC_SUCCESS;
}




/**
 * Checks to see if all tile groups in a device are finished.
 * @param[in]  device        Pointer to device
 * returns HB_MC_SUCCESS if all tile groups are finished, and HB_MC_FAIL otherwise.
 */
int hb_mc_device_all_tile_groups_finished(device_t *device) {
	
	if (hb_mc_fifo_check(device->fd) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to verify device.\n", __func__); 
		return HB_MC_FAIL;
	}

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

	if (hb_mc_fifo_check(device->fd) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to verify device.\n", __func__); 
		return HB_MC_FAIL;
	}

	int tile_group_finished = 0;
	hb_mc_request_packet_t recv, finish;
	uint32_t intf_coord_x, intf_coord_y;
	int error; 

	error = hb_mc_get_config(device->fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &intf_coord_x);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to get host interface X coord.\n", __func__);
		return HB_MC_FAIL;
	}

	error = hb_mc_get_config(device->fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &intf_coord_y);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to get host interface Y coord.\n", __func__);
		return HB_MC_FAIL;
	}
	
 
	while (!tile_group_finished) {
		hb_mc_fifo_receive(device->fd, HB_MC_FIFO_RX_REQ, (hb_mc_packet_t *) &recv);
		
		/* Check all tile groups to see if the received packet is the finish packet from one of them */
		tile_group_t *tg = device->tile_groups;
		for (int tg_num = 0; tg_num < device->num_tile_groups; tg_num ++, tg ++) {
			if (tg->status == HB_MC_TILE_GROUP_STATUS_LAUNCHED) {


				bsg_pr_dbg("%s: Expecting packet src (%d,%d), dst (%d, %d), addr: 0x%x, data: %d.\n", __func__, tg->origin.x, tg->origin.y, intf_coord.x, intf_coord.y, tg->kernel->finish_signal_addr, 0x1);

				hb_mc_request_packet_set_x_dst(&finish, (uint8_t) intf_coord_x);
				hb_mc_request_packet_set_y_dst(&finish, (uint8_t) intf_coord_y);
				hb_mc_request_packet_set_x_src(&finish, (uint8_t) tg->origin.x);
				hb_mc_request_packet_set_y_src(&finish, (uint8_t) tg->origin.y);
				hb_mc_request_packet_set_data(&finish, 0x1 /* TODO: Hardcoded */);
				hb_mc_request_packet_set_mask(&finish, HB_MC_PACKET_REQUEST_MASK_WORD);
				hb_mc_request_packet_set_op(&finish, HB_MC_PACKET_OP_REMOTE_STORE);
				hb_mc_request_packet_set_addr(&finish, tg->kernel->finish_signal_addr);

				if (hb_mc_request_packet_equals(&recv, &finish) == HB_MC_SUCCESS) {
		
					bsg_pr_dbg("%s: Finish packet received for grid %d tile group (%d,%d): src (%d,%d), dst (%d,%d), addr: 0x%x, data: %d.\n", __func__, tg->grid_id, tg->id.x, tg->id.y, recv.x_src, recv.y_src, recv.x_dst, recv.y_dst, recv.addr, recv.data);
					error = hb_mc_tile_group_deallocate_tiles(device, tg);
					if (error != HB_MC_SUCCESS) { 
						bsg_pr_err("%s: failed to deallocate grid %d tile group (%d,%d).\n", __func__, tg->grid_id, tg->id.x, tg->id.y);
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
	
	if (hb_mc_fifo_check(device->fd) != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to verify device.\n", __func__); 
		return HB_MC_FAIL;
	}

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

	if (device->eva_id != 0) {
		bsg_pr_err("%s: eva_id not supported.\n", __func__); 
		return HB_MC_FAIL;
	} 

	if (!mem_manager[device->eva_id])
		return HB_MC_SUCCESS; /* there is no memory manager to deinitialize */	
	delete(mem_manager[device->eva_id]);
	
	for (int i = 0; i < device->mesh->dim.x * device->mesh->dim.y ; i++) { /* freeze tiles */
		hb_mc_tile_freeze_dep(device->fd, device->mesh->tiles[i].coord.x, device->mesh->tiles[i].coord.y);
	}


	hb_mc_manycore_exit(device->mc); 


	int error = hb_mc_fifo_finish(device->fd);
	if (error != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to terminate host.\n", __func__);
		return HB_MC_FAIL;
	}


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
int hb_mc_device_malloc (device_t *device, uint32_t size, /*out*/ eva_t *eva) {
        *eva = 0;
	if (device->eva_id != 0) {
		bsg_pr_err("%s: invalid EVA ID %d.\n", __func__, device->eva_id);
		return HB_MC_FAIL; 
	}
	else if (!mem_manager[device->eva_id]) {
		bsg_pr_err("%s: memory manager not initialized.\n", __func__);
		return HB_MC_FAIL; 
	}

	eva_t result = mem_manager[device->eva_id]->alloc(size);
	if (result == awsbwhal::MemoryManager::mNull) {
		bsg_pr_err("%s: failed to allocated memory.\n", __func__);	
		return HB_MC_FAIL; 
	}
        *eva = result;
	return HB_MC_SUCCESS;
}




/*!
 * frees Hammerblade Manycore memory.
 *@param device pointer to the device.
 *@param eva address to free.
 *@return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. This function can fail if eva_id is invalid or of the memory manager corresponding to eva_id has not been initialized.
 */
int hb_mc_device_free (device_t *device, eva_t eva) {
	if (device->eva_id != 0) {
		bsg_pr_err("%s: invalid EVA ID %d.\n", __func__, device->eva_id); 
		return HB_MC_FAIL; 
	}
	else if (!mem_manager[device->eva_id]) {
		bsg_pr_err("%s: memory manager not initialized.\n", __func__);
		return HB_MC_FAIL; 
	}

	mem_manager[device->eva_id]->free(eva);
	return HB_MC_SUCCESS;
}





/*
 * caller must ensure eva_id is valid. */
static int hb_mc_cpy_to_eva (uint8_t fd, eva_id_t eva_id, eva_t dst, uint32_t *src) {
	npa_t npa;	
	int error = hb_mc_eva_to_npa_deprecated(eva_id, dst, &npa);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not convert EVA to an NPA */
	}
	error = hb_mc_copy_to_epa (fd, npa.x, npa.y, npa.epa, src, 1 /* 1 word */);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not send data to Manycore */
	}
	return HB_MC_SUCCESS;
}





/*
 * caller must esure eva_id is valid. 
 * */
static int hb_mc_cpy_from_eva (uint8_t fd, eva_id_t eva_id, hb_mc_response_packet_t *dest, eva_t src) {
	npa_t npa;	
	int error = hb_mc_eva_to_npa_deprecated(eva_id, src, &npa);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not convert EVA to an NPA */
	}
	error = hb_mc_copy_from_epa (fd, dest, npa.x, npa.y, npa.epa, 1 /* 1 word */);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not send data to Manycore */
	}
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
	if (device->eva_id != 0) {
		bsg_pr_err("%s: invalid EVA ID %d.\n", __func__, device->eva_id);
		return HB_MC_FAIL; 
	}

	else if (kind == hb_mc_memcpy_to_device) { /* copy to Manycore */
		eva_t dst_eva = (eva_t) reinterpret_cast<uintptr_t>(dst);
		for (int i = 0; i < count; i += sizeof(uint32_t)) { /* copy one word at a time */
			char *src_word = (char *) src + i;
			int error = hb_mc_cpy_to_eva(device->fd, device->eva_id, dst_eva + i, (uint32_t *) (src_word)); 		
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to copy to device.\n", __func__);
				return HB_MC_FAIL; 
			}
		}
		return HB_MC_SUCCESS;	
	}
	
	else if (kind == hb_mc_memcpy_to_host) { /* copy to Host */
		eva_t src_eva = (eva_t) reinterpret_cast<uintptr_t>(src);
		for (int i = 0; i < count; i += sizeof(uint32_t)) { /* copy one word at a time */
                        // read in a packet
                        hb_mc_response_packet_t dst_packet;
			int error = hb_mc_cpy_from_eva(device->fd, device->eva_id, &dst_packet, src_eva + i);
			if (error != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to copy to host.\n", __func__);
				return HB_MC_FAIL; 
			}

                        // copy the word into caller dst buffer
                        uint32_t *dst_w = (uint32_t*)dst;
                        dst_w[i/sizeof(uint32_t)] = hb_mc_response_packet_get_data(&dst_packet);
		}
		return HB_MC_SUCCESS;	
	}
	else {
		bsg_pr_err("%s: invalid copy type. Copy type can be one of hb_mc_memcpy_to_device or hb_mc_memcpy_to_host.\n", __func__);
		return HB_MC_FAIL; 
		}
}



