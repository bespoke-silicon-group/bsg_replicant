#include <bsg_manycore_tile.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_epa.h>
#include <bsg_manycore_loader.h>


#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

/**
 * Set the DRAM enabled bit for a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to unfreeze.
 * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_dram_enabled(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile)
{
        hb_mc_npa_t npa = hb_mc_npa(*tile, HB_MC_TILE_EPA_CSR_DRAM_ENABLE);
        return hb_mc_manycore_write32(mc, &npa, 1);
}

/**
 * Clear the DRAM enabled bit for a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to unfreeze.
 * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
 */
int hb_mc_tile_clear_dram_enabled(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile)
{
        hb_mc_npa_t npa = hb_mc_npa(*tile, HB_MC_TILE_EPA_CSR_DRAM_ENABLE);
        return hb_mc_manycore_write32(mc, &npa, 0);
}

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
int hb_mc_tile_set_origin_registers(	hb_mc_manycore_t *mc,
					const hb_mc_coordinate_t *coord,
					const hb_mc_coordinate_t *origin) {
	int error; 
	hb_mc_npa_t org_x_npa = hb_mc_npa(*coord, HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X);
	hb_mc_npa_t org_y_npa = hb_mc_npa(*coord, HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y);
	
	
	error =  hb_mc_manycore_write32(mc, &org_x_npa, origin->x);
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to set tile (%d,%d) CSR_TGO_X register.\n",
				__func__, coord->x, coord->y); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) bsg_tiles_org_X to %d.\n",
			__func__, coord->x, coord->y, origin->x);

	error = hb_mc_manycore_write32(mc, &org_y_npa, origin->y); 
	if (error != HB_MC_SUCCESS) { 
		bsg_pr_err(	"%s: failed to set tile (%d,%d) CSR_TGO_Y register.\n",
				__func__, coord->x, coord->y); 
		return error;
	}
	bsg_pr_dbg(	"%s: Setting tile (%d,%d) bsg_tiles_org_Y to %d.\n",
			__func__, coord->x, coord->y, origin->y);

	return HB_MC_SUCCESS; 
}


