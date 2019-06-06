#include <bsg_manycore_tile.h> 
#include <bsg_manycore_driver.h>
#include <bsg_manycore_epa.h>
#include <stdio.h>

/**
 * Freeze a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to freeze.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_freeze(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile)
{
	hb_mc_npa_t npa = hb_mc_npa(*tile, HB_MC_TILE_EPA_CSR_FREEZE);
	return hb_mc_manycore_write32(mc, &npa, 1);
}

/**
 * Unfreeze a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to unfreeze.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_unfreeze(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile)
{
	hb_mc_npa_t npa = hb_mc_npa(*tile, HB_MC_TILE_EPA_CSR_FREEZE);
	return hb_mc_manycore_write32(mc, &npa, 0);
}

/**
 * Set a tile's x origin
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to set the origin of.
 * @param[in] x      The X coordinate of the origin tile
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin_x(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
			const hb_mc_idx_t x)
{
	hb_mc_npa_t npa = hb_mc_npa(*tile, 
				HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X);
	return hb_mc_manycore_write32(mc, &npa, x);
}

/**
 * Set a tile's y origin
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to set the origin of.
 * @param[in] y      The Y coordinate of the origin tile
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin_y(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
			const hb_mc_idx_t y)
{
	hb_mc_npa_t npa = hb_mc_npa(*tile, 
				HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y);
	return hb_mc_manycore_write32(mc, &npa, y);
}

/**
 * Set a tile's origin
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to set the origin of.
 * @param[in] o      The origin tile
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
			const hb_mc_coordinate_t *o)
{
	int rc;
	rc = hb_mc_tile_set_origin_x(mc, tile, hb_mc_coordinate_get_x(*o));
	if(rc != HB_MC_SUCCESS){
		return rc;
	}

	rc = hb_mc_tile_set_origin_y(mc, tile, hb_mc_coordinate_get_y(*o));
	return rc;
}





/*!
 * Sets a Vanilla Core Endpoint's tile group's origin hardware registers CSR_TGO_X/Y.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] coord      Tile coordinates to set the origin of.
 * @param[in] origin     Origin coordinates.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_set_origin_registers(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *coord, const hb_mc_coordinate_t *origin) {
	int error; 
	hb_mc_npa_t org_x_npa = hb_mc_npa(*coord, HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X);
	hb_mc_npa_t org_y_npa = hb_mc_npa(*coord, HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y);
	
	
	error =  hb_mc_manycore_write32(mc, &org_x_npa, origin->x);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to set tile (%d,%d) CSR_TGO_X register.\n", __func__, coord->x, coord->y); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) bsg_tiles_org_X to %d.\n", __func__, coord->x, coord->y, origin->x);

	error = hb_mc_manycore_write32(mc, &org_y_npa, origin->y); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to set tile (%d,%d) CSR_TGO_Y register.\n", __func__, coord->x, coord->y); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) bsg_tiles_org_Y to %d.\n", __func__, coord->x, coord->y, origin->y);

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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_set_origin_symbols (hb_mc_manycore_t *mc, hb_mc_eva_map_t *map, unsigned char* bin, size_t bin_size, const hb_mc_coordinate_t *coord, const hb_mc_coordinate_t *origin){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t org_x_eva, org_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grp_org_x", &org_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: hb_mc_tile_set_origin_symbols() --> symbol_to_eva(): failed to aquire __bsg_grp_org_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grp_org_y", &org_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: hb_mc_tile_set_origin_symbols() --> symbol_to_eva(): failed to aquire __bsg_grp_org_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_npa_t org_x_npa, org_y_npa;
	size_t sz;
	error = hb_mc_eva_to_npa(cfg, map, coord, &org_x_eva, &org_x_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_grp_org_x npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}

	error = hb_mc_eva_to_npa(cfg, map, coord, &org_y_eva, &org_y_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_grp_org_y npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}


	error = hb_mc_manycore_write32(mc, &org_x_npa, hb_mc_coordinate_get_x(*origin));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_grp_org_x to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_grp_org_x (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), org_x_npa.epa, hb_mc_coordinate_get_x(*origin));


	error = hb_mc_manycore_write32(mc, &org_y_npa, hb_mc_coordinate_get_y(*origin));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_grp_org_y to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_grp_org_y (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), org_y_npa.epa, hb_mc_coordinate_get_y(*origin));

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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_set_coord_symbols (hb_mc_manycore_t *mc, hb_mc_eva_map_t *map, unsigned char* bin, size_t bin_size, const hb_mc_coordinate_t *coord, const hb_mc_coordinate_t *coord_val){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t bsg_x_eva, bsg_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_x", &bsg_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_y", &bsg_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_npa_t bsg_x_npa, bsg_y_npa;
	size_t sz;
	error = hb_mc_eva_to_npa(cfg, map, coord, &bsg_x_eva, &bsg_x_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_x npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}

	error = hb_mc_eva_to_npa(cfg, map, coord, &bsg_y_eva, &bsg_y_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_y npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}


	error = hb_mc_manycore_write32(mc, &bsg_x_npa, hb_mc_coordinate_get_x(*coord_val));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_x to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg__x (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), bsg_x_npa.epa, hb_mc_coordinate_get_x(*coord_val));


	error = hb_mc_manycore_write32(mc, &bsg_y_npa, hb_mc_coordinate_get_y(*coord_val));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_y to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_y (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), bsg_y_npa.epa, hb_mc_coordinate_get_y(*coord_val));

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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_id_symbol (hb_mc_manycore_t *mc, hb_mc_eva_map_t *map, unsigned char* bin, size_t bin_size, const hb_mc_coordinate_t *coord, const hb_mc_coordinate_t *coord_val, const hb_mc_dimension_t *dim){


	int error;

	hb_mc_idx_t id = hb_mc_coordinate_get_y(*coord_val) * hb_mc_dimension_get_x(*dim) + hb_mc_coordinate_get_x(*coord_val); /* calculate tile's id */

	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t bsg_id_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_id", &bsg_id_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s:: failed to aquire __bsg_id eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_npa_t bsg_id_npa;
	size_t sz;
	error = hb_mc_eva_to_npa(cfg, map, coord, &bsg_id_eva, &bsg_id_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_id npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}


	error = hb_mc_manycore_write32(mc, &bsg_id_npa, id);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_id to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_id (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), bsg_id_npa.epa, id);

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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_tile_group_id_symbols (hb_mc_manycore_t *mc, hb_mc_eva_map_t *map, unsigned char* bin, size_t bin_size, const hb_mc_coordinate_t *coord, const hb_mc_coordinate_t *tg_id){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t tg_id_x_eva, tg_id_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_tile_group_id_x", &tg_id_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_tile_group_id_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_tile_group_id_y", &tg_id_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_tile_group_id_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_npa_t tg_id_x_npa, tg_id_y_npa;
	size_t sz;
	error = hb_mc_eva_to_npa(cfg, map, coord, &tg_id_x_eva, &tg_id_x_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_tile_group_id_x npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}

	error = hb_mc_eva_to_npa(cfg, map, coord, &tg_id_y_eva, &tg_id_y_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_tile_group_id_y npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}


	error = hb_mc_manycore_write32(mc, &tg_id_x_npa, hb_mc_coordinate_get_x(*tg_id));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_tile_group_id_x to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_tile_group_id_x (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), tg_id_x_npa.epa, hb_mc_coordinate_get_x(*tg_id));


	error = hb_mc_manycore_write32(mc, &tg_id_y_npa, hb_mc_coordinate_get_y(*tg_id));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_tile_group_id_y to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_tile_group_id_y (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), tg_id_y_npa.epa, hb_mc_coordinate_get_y(*tg_id));

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
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_grid_dim_symbols (hb_mc_manycore_t *mc, hb_mc_eva_map_t *map, unsigned char* bin, size_t bin_size, const hb_mc_coordinate_t *coord, const hb_mc_dimension_t *grid_dim){

	int error;
	const hb_mc_config_t *cfg = hb_mc_manycore_get_config (mc); 

	hb_mc_eva_t grid_dim_x_eva, grid_dim_y_eva;
	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grid_dim_x", &grid_dim_x_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_grid_dim_x eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}

	error = hb_mc_loader_symbol_to_eva(bin, bin_size, "__bsg_grid_dim_y", &grid_dim_y_eva); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_grid_dim_y eva.\n", __func__);
		return HB_MC_NOTFOUND;
	}


	hb_mc_npa_t grid_dim_x_npa, grid_dim_y_npa;
	size_t sz;
	error = hb_mc_eva_to_npa(cfg, map, coord, &grid_dim_x_eva, &grid_dim_x_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_grid_dim_x npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}

	error = hb_mc_eva_to_npa(cfg, map, coord, &grid_dim_y_eva, &grid_dim_y_npa, &sz);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to aquire __bsg_grid_dim_y npa from eva.\n", __func__);
		return HB_MC_FAIL;
	}


	error = hb_mc_manycore_write32(mc, &grid_dim_x_npa, hb_mc_dimension_get_x(*grid_dim));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_grid_dim_x to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_gird_dim_x (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), grid_dim_x_npa.epa, hb_mc_dimension_get_x(*grid_dim));


	error = hb_mc_manycore_write32(mc, &grid_dim_y_npa, hb_mc_dimension_get_y(*grid_dim));
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err("%s: failed to write __bsg_grid_dim_y to tile (%d,%d).\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord)); 
		return HB_MC_FAIL;
	}
	bsg_pr_dbg("%s: Setting tile (%d,%d) __bsg_grid_dim_y (epa 0x%x) to %d.\n", __func__, hb_mc_coordinate_get_x(*coord), hb_mc_coordinate_get_y(*coord), grid_dim_y_npa.epa, hb_mc_dimension_get_y(*grid_dim));

	return HB_MC_SUCCESS;
}




/*! 
 * Writes a uint8_t into a tile given its coordinates and epa 
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] coord      Cooridnates of the destination tile
 * @param[in] epa        Epa address in desitnation tile to which the write occurs
 * @param[in] v          Variable to be written into tile 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_write8(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *coord, const hb_mc_epa_t *epa, uint8_t v) { 
	hb_mc_npa_t npa = hb_mc_npa(*coord, *epa);
	return hb_mc_manycore_write8(mc, &npa, v); 
}



/*! 
 * Writes a uint16_t into a tile given its coordinates and epa 
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] coord      Cooridnates of the destination tile
 * @param[in] epa        Epa address in desitnation tile to which the write occurs
 * @param[in] v          Variable to be written into tile 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_write16(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *coord, const hb_mc_epa_t *epa, uint16_t v) { 
	hb_mc_npa_t npa = hb_mc_npa(*coord, *epa);
	return hb_mc_manycore_write16(mc, &npa, v); 
}



/*! 
 * Writes a uint32_t into a tile given its coordinates and epa 
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] coord      Cooridnates of the destination tile
 * @param[in] epa        Epa address in desitnation tile to which the write occurs
 * @param[in] v          Variable to be written into tile 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_write32(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *coord, const hb_mc_epa_t *epa, uint32_t v) { 
	hb_mc_npa_t npa = hb_mc_npa(*coord, *epa);
	return hb_mc_manycore_write32(mc, &npa, v); 
}




/*! 
 * Writes a buffer into a tile given its coordinates and epa 
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] coord      Cooridnates of the destination tile
 * @param[in] epa        Epa address in desitnation tile to which the write occurs
 * @param[in] vp         Buffer to be written into tile 
 * @param[in] sz         Size of buffer to be written
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_write(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *coord, const hb_mc_epa_t *epa, const void* vp, size_t sz) { 
	hb_mc_npa_t npa = hb_mc_npa(*coord, *epa);
	return hb_mc_manycore_write_mem(mc, &npa, vp, sz); 
}




