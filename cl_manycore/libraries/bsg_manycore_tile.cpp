#include <bsg_manycore_tile.h> 
#include <bsg_manycore_driver.h>
#include <bsg_manycore_epa.h>

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
				HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X);
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
 * Freezes a Vanilla Core Endpoint.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_freeze_dep (uint8_t fd, uint8_t x, uint8_t y) {
	hb_mc_packet_t freeze; 
	epa_t freeze_waddr;
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	freeze_waddr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_CSR_BASE, 
					HB_MC_TILE_EPA_CSR_FREEZE_OFFSET);
		
	hb_mc_format_request_packet(fd,
                                    &freeze.request,
                                    freeze_waddr,
                                    HB_MC_CSR_FREEZE,
                                    x, y, HB_MC_PACKET_OP_REMOTE_STORE);
	if (hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &freeze) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	else
		return HB_MC_SUCCESS;
}

/*!
 * Unfreezes a Vanilla Core Endpoint.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_unfreeze_dep (uint8_t fd, uint8_t x, uint8_t y) {
	hb_mc_packet_t unfreeze; 
	epa_t freeze_waddr;
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	freeze_waddr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_CSR_BASE, 
						HB_MC_TILE_EPA_CSR_FREEZE_OFFSET);
		
	hb_mc_format_request_packet(fd,
                                    &unfreeze.request, 
                                    freeze_waddr,
                                    HB_MC_CSR_UNFREEZE, 
                                    x, y, HB_MC_PACKET_OP_REMOTE_STORE);
	if (hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &unfreeze) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	else
		return HB_MC_SUCCESS;
}


/*!
 * Sets a Vanilla Core Endpoint's tile group's origin.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] origin_x x coordinate of tile group's origin
 * @param[in] origin_y y coordinate of tile groups origin
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_set_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t origin_x, uint8_t origin_y) {
	hb_mc_packet_t packet_origin_x, packet_origin_y;
	epa_t x_origin_addr, y_origin_addr;
	int rc;
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	x_origin_addr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_CSR_BASE,
						HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET);
	hb_mc_format_request_packet(fd,
                                    &packet_origin_x.request, 
                                    x_origin_addr,
                                    origin_x, x, y,
                                    HB_MC_PACKET_OP_REMOTE_STORE);
	rc = hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &packet_origin_x);
	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	y_origin_addr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_CSR_BASE,
						HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET);
	hb_mc_format_request_packet(fd,
                                    &packet_origin_y.request,
                                    y_origin_addr,
                                    origin_y, x, y, 
                                    HB_MC_PACKET_OP_REMOTE_STORE);
	rc = hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &packet_origin_y);

	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	return HB_MC_SUCCESS;
}
