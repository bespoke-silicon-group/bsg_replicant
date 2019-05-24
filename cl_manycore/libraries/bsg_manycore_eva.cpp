#define DEBUG
#include "bsg_manycore_eva.h"
#include <math.h>
#ifndef COSIM
#include <bsg_manycore_printing.h>
#include <bsg_manycore_coordinate.h>
#else
#include "bsg_manycore_printing.h"
#include "bsg_manycore_coordinate.h"
#endif

#define MAKE_MASK(WIDTH) ((1ULL << WIDTH) - 1)


// Tile Memory default is 4096 bytes
#define DEFAULT_TILE_DMEM_WIDTH 12
#define DEFAULT_TILE_DMEM_BITMASK MAKE_MASK(DEFAULT_TILE_DMEM_WIDTH)
#define DEFAULT_TILE_DMEM_SIZE (1ULL << DEFAULT_TILE_DMEM_WIDTH)
#define DEFAULT_TILE_CSR_WIDTH 17
#define DEFAULT_TILE_FREEZE_ADDR (1ULL << DEFAULT_TILE_CSR_WIDTH)
#define DEFAULT_TILE_ORIGIN_X_ADDR (DEFAULT_TILE_FREEZE_ADDR + 4)
#define DEFAULT_TILE_ORIGIN_Y_ADDR (DEFAULT_TILE_FREEZE_ADDR + 8)

// However, 18 bits are exposed
#define DEFAULT_LOCAL_WIDTH 18
#define DEFAULT_LOCAL_BITMASK MAKE_MASK(DEFAULT_LOCAL_WIDTH)

#define DEFAULT_GROUP_BITIDX 29
#define DEFAULT_GROUP_BITMASK (1ULL << DEFAULT_GROUP_BITIDX)
#define DEFAULT_GROUP_EPA_BITMASK DEFAULT_TILE_DMEM_BITMASK
#define DEFAULT_GROUP_EPA_SIZE DEFAULT_TILE_DMEM_SIZE

#define DEFAULT_GROUP_X_WIDTH 6
#define DEFAULT_GROUP_X_BITIDX DEFAULT_LOCAL_WIDTH
#define DEFAULT_GROUP_X_BITMASK (MAKE_MASK(DEFAULT_GROUP_X_WIDTH) << DEFAULT_GROUP_X_BITIDX)

#define DEFAULT_GROUP_Y_WIDTH 5
#define DEFAULT_GROUP_Y_BITIDX (DEFAULT_GROUP_X_BITIDX + DEFAULT_GROUP_X_WIDTH)
#define DEFAULT_GROUP_Y_BITMASK (MAKE_MASK(DEFAULT_GROUP_Y_WIDTH) << DEFAULT_GROUP_Y_BITIDX)

#define DEFAULT_GLOBAL_BITIDX 30
#define DEFAULT_GLOBAL_BITMASK (1ULL << DEFAULT_GLOBAL_BITIDX)
#define DEFAULT_GLOBAL_EPA_BITMASK DEFAULT_TILE_DMEM_BITMASK
#define DEFAULT_GLOBAL_EPA_SIZE DEFAULT_TILE_DMEM_SIZE

#define DEFAULT_GLOBAL_X_WIDTH 6
#define DEFAULT_GLOBAL_X_BITIDX DEFAULT_LOCAL_WIDTH
#define DEFAULT_GLOBAL_X_BITMASK (MAKE_MASK(DEFAULT_GLOBAL_X_WIDTH) << DEFAULT_GLOBAL_X_BITIDX)

#define DEFAULT_GLOBAL_Y_WIDTH 6
#define DEFAULT_GLOBAL_Y_BITIDX (DEFAULT_GLOBAL_X_BITIDX + DEFAULT_GLOBAL_X_WIDTH)
#define DEFAULT_GLOBAL_Y_BITMASK (MAKE_MASK(DEFAULT_GLOBAL_Y_WIDTH) << DEFAULT_GLOBAL_Y_BITIDX)

#define DEFAULT_DRAM_BITIDX 31
#define DEFAULT_DRAM_BITMASK (1ULL << DEFAULT_DRAM_BITIDX)

/**
 * Determines if an EVA is a tile-local EVA
 * @return true if EVA addresses tile-local memory, false otherwise
 */
static bool default_eva_is_local(const hb_mc_eva_t *eva)
{
	return !(hb_mc_eva_addr(eva) & ~(DEFAULT_LOCAL_BITMASK));
}

/**
 * Converts a tile-local EVA to an NPA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  c      A target tile to compute #npa
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva and #id
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return true if EVA addresses tile-local memory, false otherwise
 */
static int default_eva_to_npa_local(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *c,
				const hb_mc_eva_t *eva, hb_mc_npa_t *npa, size_t *sz)
{
	bsg_pr_dbg("%s: Translating EVA 0x%x to NPA\n", __func__, *eva);
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;
	x = hb_mc_coordinate_get_x(*c);
	y = hb_mc_coordinate_get_y(*c);
	epa = hb_mc_eva_addr(eva) & DEFAULT_LOCAL_BITMASK;
	// EVA is writing to DMEM
	if(epa < DEFAULT_TILE_DMEM_SIZE){
		*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
		*sz = DEFAULT_TILE_DMEM_SIZE - epa;
	}else if(epa == DEFAULT_TILE_FREEZE_ADDR){
		*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
		*sz = sizeof(uint32_t);
	}else if(epa == DEFAULT_TILE_ORIGIN_X_ADDR){
		*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
		*sz = sizeof(uint32_t);
	}else if(epa == DEFAULT_TILE_ORIGIN_Y_ADDR){
		*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
		*sz = sizeof(uint32_t);
	} else {
		bsg_pr_err("%s: Invalid EVA Address 0x%x\n", __func__, *eva);
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

/**
 * Determines if an EVA is a group EVA
 * @return true if EVA addresses group memory, false otherwise
 */
static bool default_eva_is_group(const hb_mc_eva_t *eva)
{
	return (hb_mc_eva_addr(eva) & DEFAULT_GROUP_BITMASK) != 0;
}

/**
 * Converts a group-local EVA to an NPA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  c      A target tile to compute #npa (not used)
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva and #id
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_group(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *c,
				const hb_mc_eva_t *eva, hb_mc_npa_t *npa, size_t *sz)
{
	bsg_pr_dbg("%s: Translating EVA 0x%x to NPA\n", __func__, *eva);
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;
	x = ((hb_mc_eva_addr(eva) & DEFAULT_GROUP_X_BITMASK) >> DEFAULT_GROUP_X_BITIDX);
	y = ((hb_mc_eva_addr(eva) & DEFAULT_GROUP_Y_BITMASK) >> DEFAULT_GROUP_Y_BITIDX);
	epa = hb_mc_eva_addr(eva) & DEFAULT_GROUP_EPA_BITMASK;
	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
	*sz = DEFAULT_GROUP_EPA_SIZE - epa;
	return HB_MC_SUCCESS;
}

/**
 * Determines if an EVA is a global EVA
 * @return true if EVA addresses global memory, false otherwise
 */
static bool default_eva_is_global(const hb_mc_eva_t *eva)
{
	return (hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_BITMASK) != 0;
}

/**
 * Converts a global EVA to an NPA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  c      A target tile to compute #npa (not used)
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva and #id
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_global(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *c,
				const hb_mc_eva_t *eva, hb_mc_npa_t *npa, size_t *sz)
{
	bsg_pr_dbg("%s: Translating EVA 0x%x to NPA\n", __func__, *eva);
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;
	x = ((hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_X_BITMASK) >> DEFAULT_GLOBAL_X_BITIDX);
	y = ((hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_Y_BITMASK) >> DEFAULT_GLOBAL_Y_BITIDX);
	epa = hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_EPA_BITMASK;
	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
	*sz = DEFAULT_GLOBAL_EPA_SIZE - epa;
	return HB_MC_SUCCESS;
}

/**
 * Determines if an EVA is in DRAM
 * @return true if EVA addresses DRAM memory, false otherwise
 */
static bool default_eva_is_dram(const hb_mc_eva_t *eva)
{
	return (hb_mc_eva_addr(eva) & DEFAULT_DRAM_BITMASK) != 0;
}

/**
 * Converts a DRAM EVA to an NPA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  c      A target tile to compute #npa (not used)
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva and #id
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_dram(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *c,
				const hb_mc_eva_t *eva, hb_mc_npa_t *npa, size_t *sz)
{
	bsg_pr_dbg("%s: Translating EVA 0x%x to NPA\n", __func__, *eva);
	uint32_t mask, shift, clogxdim;
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;
	hb_mc_dimension_t dim;
	dim = hb_mc_config_get_dimension(cfg);

	// The number of bits used for the x index, and the location of the x
	// index in the eva is determined by clog2 of the x dimension (or the
	// number of bits needed to represent the maximum x dimension).
	clogxdim = ceil(log2(hb_mc_dimension_get_x(dim)));

	shift = DEFAULT_DRAM_BITIDX - clogxdim;
	mask = MAKE_MASK(clogxdim);

	x = (hb_mc_eva_addr(eva) >> shift) & mask;
	y = hb_mc_dimension_get_y(dim) + 1;

	epa = (*eva & MAKE_MASK(shift));

	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);
	// Likewise, the size of the NPA segment is determined by the number of
	// bits not used by the x dimension (plus 1 for the top bit that
	// indicates this EVA is for DRAM)
	*sz = (1 << shift) - epa;
	return HB_MC_SUCCESS;
}

/**
 * Converts a EVA to an NPA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  c      A target tile to compute #npa
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa(const hb_mc_config_t *cfg,
			const hb_mc_coordinate_t *c,
		const hb_mc_eva_t *eva, hb_mc_npa_t *npa, size_t *sz)
{
	if(default_eva_is_dram(eva))
		return default_eva_to_npa_dram(cfg, c, eva, npa, sz);
	if(default_eva_is_global(eva))
		return default_eva_to_npa_global(cfg, c, eva, npa, sz);
	if(default_eva_is_group(eva))
		return default_eva_to_npa_group(cfg, c, eva, npa, sz);
	if(default_eva_is_local(eva))
		return default_eva_to_npa_local(cfg, c, eva, npa, sz);
	bsg_pr_err("%s: EVA did not map to a known region\n", __func__);
	return HB_MC_FAIL;
}

/**
 * Converts a NPA to a EVA in a coordinate's address space.
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  c      A target tile to compute #eva
 * @param[in]  npa    An npa to translate
 * @param[out] eva    An eva to set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_npa_to_eva(const hb_mc_config_t *cfg,
			const hb_mc_coordinate_t *c,
			const hb_mc_npa_t *npa, hb_mc_eva_t *eva, size_t *sz)
{
	/*
	if(default_npa_is_dram(npa, c))
		return default_npa_to_eva_dram(cfg, c, npa, eva, sz);
	if(default_npa_is_global(npa, c))
		return default_npa_to_eva_global(cfg, c, npa, eva, sz);
	if(default_npa_is_group(npa, c))
		return default_npa_to_eva_group(cfg, c, npa, eva, sz);
	if(default_npa_is_local(npa, c))
		return default_npa_to_eva_local(cfg, c, npa, eva, sz);
	*/
	return HB_MC_FAIL;
}

hb_mc_eva_id_t default_eva = {
	.eva_id_name = "default EVA space",
	.eva_to_npa  = default_eva_to_npa,
	.npa_to_eva  = default_npa_to_eva
};

/**
 * Translates a Network Physical Address to an Endpoint Virtual Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A coordinate for which #eva will be formatted
 * @param[in]  npa    A valid npa_t to translate
 * @param[out] eva    An eva to be set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_npa_to_eva(const hb_mc_config_t *cfg,
		const hb_mc_eva_id_t *id, const hb_mc_coordinate_t *c,
		const hb_mc_npa_t *npa, hb_mc_eva_t *eva, size_t *sz)
{
	int err;

	err = id->npa_to_eva(cfg, c, npa, eva, sz);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A target tile to compute #npa
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva and #id
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_eva_to_npa(const hb_mc_config_t *cfg,
		     const hb_mc_eva_id_t *id,
		     const hb_mc_coordinate_t *c, const hb_mc_eva_t *eva,
		     hb_mc_npa_t *npa, size_t *sz)
{
	int err;

	err = id->eva_to_npa(cfg, c, eva, npa, sz);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}


/**
 * Write memory out to manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A target tile to compute the NPA
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_write(const hb_mc_manycore_t *mc,
			const hb_mc_eva_id_t *id,
			const hb_mc_coordinate_t *c, const hb_mc_eva_t *eva,
			const void *data, size_t sz)
{
	int err;

	err = HB_MC_FAIL;// TODO: Check that (sz, eva, id, coord) is valid
	if (err != HB_MC_SUCCESS)
		return err;

	// TODO: Perform a memcopy for each EVA -> NPA Segment
	return HB_MC_SUCCESS;
}

/**
 * Read memory from manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A coordinate on the the Manycore
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from the manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_read(const hb_mc_manycore_t *mc,
			const hb_mc_eva_id_t *id,
			const hb_mc_coordinate_t *c, const hb_mc_eva_t *eva,
			void *data, size_t sz)
{
	int err;

	err = HB_MC_FAIL;// TODO: Check that (sz, eva, id, coord) is valid
	if (err != HB_MC_SUCCESS)
		return err;

	// TODO: Perform a memcopy for each EVA -> NPA Segment
	return HB_MC_SUCCESS;
}
