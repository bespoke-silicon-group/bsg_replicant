#include <stdio.h>

#ifndef COSIM
	#include <bsg_manycore_tile.h> 
#else
	#include "bsg_manycore_tile.h"
#endif

/*!
 * Freezes a Vanilla Core Endpoint.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_freeze (uint8_t fd, uint8_t x, uint8_t y) {
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
int hb_mc_tile_unfreeze (uint8_t fd, uint8_t x, uint8_t y) {
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
 * Sets a Vanilla Core Endpoint's tile group's origin hardware registers CSR_TGO_X/Y.
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id eva-to-npa mapping
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] origin_x x coordinate of tile group's origin
 * @param[in] origin_y y coordinate of tile groups origin
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_set_origin_registers(uint8_t fd, uint32_t x, uint32_t y, uint32_t origin_x, uint32_t origin_y) {
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_origin_registers(): invalid device %d.\n", fd);
		return HB_MC_FAIL;
	}


	if (hb_mc_copy_to_epa(fd, x, y, hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_CSR_BASE, HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET), &origin_x, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_origin_registers() --> hb_mc_copy_to_epa(): failed to set tile X origin.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) bsg_tiles_org_X to %d.\n", x, y, origin_x);
	#endif

	if (hb_mc_copy_to_epa(fd, x, y, hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_CSR_BASE, HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET), &origin_y, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_origin_registers() --> hb_mc_copy_to_epa(): failed to set tile Y origin.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
	fprintf(stderr, "Setting tile (%d,%d) bsg_tiles_org_Y to %d.\n", x, y, origin_y);
	#endif

	return HB_MC_SUCCESS;
}

/*!
 * Sets a Vanilla Core Endpoint's tile group's origin symbols __bsg_grp_org_x/y.
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id eva-to-npa mapping
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] origin_x x coordinate of tile group's origin
 * @param[in] origin_y y coordinate of tile groups origin
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_tile_set_origin_symbols (uint8_t fd, eva_id_t eva_id, char* elf,  uint32_t x, uint32_t y, uint32_t origin_x, uint32_t origin_y){
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_origin_symbols(): invalid device %d.\n", fd);
		return HB_MC_FAIL;
	}

	eva_t bsg_origin_x_eva, bsg_origin_y_eva;
	if (symbol_to_eva(elf, "__bsg_grp_org_x", &bsg_origin_x_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_origin_symbols() --> symbol_to_eva(): failed to aquire __bsg_grp_org_x eva.\n");
		return HB_MC_FAIL;
	}
	if (symbol_to_eva(elf, "__bsg_grp_org_y", &bsg_origin_y_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_origin_symbols() --> symbol_to_eva(): failed to aquire __bsg_grp_org_y eva.\n");
		return HB_MC_FAIL;
	}

	if (hb_mc_copy_to_epa(fd, x, y, bsg_origin_x_eva >> 2 /* TODO: magic number */, &origin_x, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_origin_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_grp_org_x symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_grp_org_x (eva 0x%x) to %d.\n", x, y, bsg_origin_x_eva, origin_x);
	#endif

	if (hb_mc_copy_to_epa(fd, x, y, bsg_origin_y_eva >> 2 /* TODO: magic number */, &origin_y, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_origin_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_grp_org_y symbol .\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_grp_org_y (eva 0x%x) to %d.\n", x, y, bsg_origin_y_eva, origin_y);
	#endif

	return HB_MC_SUCCESS;
}

/*! 
 * Sets a Vanilla Core Endpoint's tile's X,Y coordinates.
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id eva-to-npa mapping
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_coord_symbols (uint8_t fd, eva_id_t eva_id, char* elf,  uint32_t x, uint32_t y, uint32_t coord_x, uint32_t coord_y){
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_coord_symbols(): invalid device %d.\n", fd);
		return HB_MC_FAIL;
	}

	eva_t bsg_x_eva, bsg_y_eva;
	if (symbol_to_eva(elf, "__bsg_x", &bsg_x_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_coord_symbols() --> symbol_to_eva(): failed to aquire __bsg_x eva.\n");
		return HB_MC_FAIL;
	}
	if (symbol_to_eva(elf, "__bsg_y", &bsg_y_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_coord_symbols() --> symbol_to_eva(): failed to aquire __bsg_y eva.\n");
		return HB_MC_FAIL;
	}

	if (hb_mc_copy_to_epa(fd, x, y, bsg_x_eva >> 2 /* TODO: magic number */, &coord_x, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_coord_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_y symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_x (eva 0x%x) to %d.\n", x, y, bsg_x_eva, coord_x);
	#endif

	if (hb_mc_copy_to_epa(fd, x, y, bsg_y_eva >> 2 /* TODO: magic number */, &coord_y, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_coord_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_y symbol .\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_y (eva 0x%x) to %d.\n", x, y, bsg_y_eva, coord_y);
	#endif

	return HB_MC_SUCCESS;
}


/*! 
 * Sets a Vanilla Core Endpoint's tile's __bsg_id symbol.
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id eva-to-npa mapping
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] dim_x x dimension of tile group.
 * @param[in] dim_y y dimension of tile group.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_id_symbol (uint8_t fd, eva_id_t eva_id, char* elf,  uint32_t x, uint32_t y, uint32_t coord_x, uint32_t coord_y, uint32_t dim_x, uint32_t dim_y){
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_id_symbol(): invalid device %d.\n", fd);
		return HB_MC_FAIL;
	}

	uint32_t id = coord_y * dim_x + coord_x; /* calculate tile's id */

	eva_t bsg_id_eva;
	if (symbol_to_eva(elf, "__bsg_id", &bsg_id_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_id_symbol() --> symbol_to_eva(): failed to aquire __bsg_id eva.\n");
		return HB_MC_FAIL;
	}

	if (hb_mc_copy_to_epa(fd, x, y, bsg_id_eva >> 2 /* TODO: magic number */, &id, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_id_symbol() --> hb_mc_copy_to_epa(): failed to set tile __bsg_id symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_id (eva 0x%x) to %d.\n", x, y, bsg_id_eva, id);
	#endif

	return HB_MC_SUCCESS;
}

/*! 
 * Sets a Vanilla Core Endpoint's tile's __bsg_tile_group_id_x/y symbol.
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id eva-to-npa mapping
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] tg_id_x/y tile group x/y id.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_tile_group_id_symbols (uint8_t fd, eva_id_t eva_id, char* elf,  uint32_t x, uint32_t y, uint32_t tg_id_x, uint32_t tg_id_y){
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_tile_group_id_symbols(): invalid device %d.\n", fd);
		return HB_MC_FAIL;
	}

	eva_t bsg_tile_group_id_x_eva, bsg_tile_group_id_y_eva;
	if (symbol_to_eva(elf, "__bsg_tile_group_id_x", &bsg_tile_group_id_x_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_tile_group_id_symbols() --> symbol_to_eva(): failed to aquire __bsg_tile_group_id_x eva.\n");
		return HB_MC_FAIL;
	}
	if (symbol_to_eva(elf, "__bsg_tile_group_id_y", &bsg_tile_group_id_y_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_tile_group_id_symbols() --> symbol_to_eva(): failed to aquire __bsg_tile_group_id_y eva.\n");
		return HB_MC_FAIL;
	}

	if (hb_mc_copy_to_epa(fd, x, y, bsg_tile_group_id_x_eva >> 2 /* TODO: magic number */, &tg_id_x, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_tile_group_id_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_tile_group_id_x symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_tile_group_id_x (eva 0x%x) to %d.\n", x, y, bsg_tile_group_id_x_eva, tg_id_x);
	#endif

	if (hb_mc_copy_to_epa(fd, x, y, bsg_tile_group_id_y_eva >> 2 /* TODO: magic number */, &tg_id_y, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_tile_group_id_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_tile_group_id_y symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_tile_group_id_y (eva 0x%x) to %d.\n", x, y, bsg_tile_group_id_y_eva, tg_id_y);
	#endif

	return HB_MC_SUCCESS;
}

/*! 
 * Sets a Vanilla Core Endpoint's tile's __bsg_grid_dim_x/y symbol.
 * @param[in] fd userspace file descriptor
 * @param[in] eva_id eva-to-npa mapping
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] grid_dim_x/y tile group's grid dimensions.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
* */
int hb_mc_tile_set_grid_dim_symbols (uint8_t fd, eva_id_t eva_id, char* elf,  uint32_t x, uint32_t y, uint32_t grid_dim_x, uint32_t grid_dim_y){
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_grid_dim_symbols(): invalid device %d.\n", fd);
		return HB_MC_FAIL;
	}

	eva_t bsg_grid_dim_x_eva, bsg_grid_dim_y_eva;
	if (symbol_to_eva(elf, "__bsg_grid_dim_x", &bsg_grid_dim_x_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_grid_dim_symbols() --> symbol_to_eva(): failed to aquire __bsg_grid_dim_x eva.\n");
		return HB_MC_FAIL;
	}

	if (symbol_to_eva(elf, "__bsg_grid_dim_y", &bsg_grid_dim_y_eva) != HB_MC_SUCCESS){
		fprintf(stderr, "hb_mc_tile_set_grid_dim_symbols() --> symbol_to_eva(): failed to aquire __bsg_grid_dim_y eva.\n");
		return HB_MC_FAIL;
	}

	if (hb_mc_copy_to_epa(fd, x, y, bsg_grid_dim_x_eva >> 2 /* TODO: magic number */, &grid_dim_x, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_grid_dim_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_grid_dim_x symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_grid_dim_x (eva 0x%x) to %d.\n", x, y, bsg_grid_dim_x_eva, grid_dim_x);
	#endif

	if (hb_mc_copy_to_epa(fd, x, y, bsg_grid_dim_y_eva >> 2 /* TODO: magic number */, &grid_dim_y, 1) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_tile_set_grid_dim_symbols() --> hb_mc_copy_to_epa(): failed to set tile __bsg_grid_dim_y symbol.\n"); 
		return HB_MC_FAIL;
	}
	#ifdef DEBUG
		fprintf(stderr, "Setting tile (%d,%d) __bsg_grid_dim_y (eva 0x%x) to %d.\n", x, y, bsg_grid_dim_y_eva, grid_dim_y);
	#endif

	return HB_MC_SUCCESS;
}

