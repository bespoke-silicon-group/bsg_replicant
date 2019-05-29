#ifndef BSG_MANYCORE_TILE_H
#define BSG_MANYCORE_TILE_H
#include <bsg_manycore_features.h>
#include <bsg_manycore.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum __hb_mc_csr_freeze_t{
        HB_MC_CSR_FREEZE = 1,
        HB_MC_CSR_UNFREEZE = 0
} hb_mc_csr_freeze_t;

/*************/
/* Tile EPAs */
/*************/

/* Offsets in bytes */
#define HB_MC_TILE_EPA_DMEM_BASE   0x00001000
#define HB_MC_TILE_EPA_ICACHE_BASE 0x01000000
#define HB_MC_TILE_EPA_CSR_BASE                       0x20000
#define HB_MC_TILE_EPA_CSR_FREEZE_OFFSET              0x00
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET 0x04
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET 0x08

#define HB_MC_TILE_DMEM_LOGSZ 12
#define HB_MC_TILE_DMEM_SIZE (1 << HB_MC_TILE_DMEM_LOGSZ)

#define EPA_TILE_CSR_FROM_BYTE_OFFSET(offset)				\
	EPA_FROM_BASE_AND_OFFSET(HB_MC_TILE_EPA_CSR_BASE, offset)

/* EPAs */
#define HB_MC_TILE_EPA_ICACHE					\
	EPA_FROM_BASE_AND_OFFSET(HB_MC_TILE_EPA_ICACHE_BASE, 0)

#define HB_MC_TILE_EPA_CSR_FREEZE					\
	EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_FREEZE_OFFSET)

#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X				\
	EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET)

#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y				\
	EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET)


/**
 * Set a tile's x origin
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to set the origin of.
 * @param[in] x      The X coordinate of the origin tile
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin_x(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
			const hb_mc_idx_t x);
/**
 * Set a tile's y origin
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to set the origin of.
 * @param[in] y      The Y coordinate of the origin tile
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin_y(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
			const hb_mc_idx_t y);
/**
 * Set a tile's origin
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to set the origin of.
 * @param[in] o      The origin tile
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
int hb_mc_tile_set_origin(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
			const hb_mc_coordinate_t *o);

/**
 * Freeze a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to freeze.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((warn_unused_result))
int hb_mc_tile_freeze(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

/**
 * Unfreeze a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to unfreeze.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((warn_unused_result))
int hb_mc_tile_unfreeze(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

/**
 * Get the size of a tiles local data memory.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  tile   The coordinate tile to query for its data memory size.
 * @return the size of the tiles data memory.
 */
static inline size_t hb_mc_tile_get_size_dmem(const hb_mc_manycore_t *mc,
					const hb_mc_coordinate_t *tile)
{
	return 4 * (1<<10); //4K -- this might be later read from the config
}

/**
 * Get the size of a tiles local data memory.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  tile   The coordinate tile to query for its instruction cache size.
 * @return the size of the tiles instruction cache.
 */
static inline size_t hb_mc_tile_get_size_icache(const hb_mc_manycore_t *mc,
						const hb_mc_coordinate_t *tile)
{
	return 4 * (1<<10); //4K -- this might be later read from the config
}

/** Deprecated **/
#define hb_mc_tile_epa_get_byte_addr(base, offset) (base + offset)

#define hb_mc_tile_epa_get_word_addr(base, offset) \
	(hb_mc_tile_epa_get_byte_addr(base, offset) >> 2)

int hb_mc_tile_freeze_dep (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_tile_unfreeze_dep (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_tile_set_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t x_cord, uint8_t y_cord);

#ifdef __cplusplus
}
#endif

#endif
