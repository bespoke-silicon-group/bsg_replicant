#include <bsg_manycore_eva.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_vcache.h>
#include <bsg_manycore_printing.h>


#ifdef __cplusplus
#include <cmath>
#else
#include <math.h>
#endif



#define MAKE_MASK(WIDTH) ((1ULL << WIDTH) - 1)

#define DEFAULT_GROUP_X_LOGSZ 6
#define DEFAULT_GROUP_X_BITIDX HB_MC_EPA_LOGSZ
#define DEFAULT_GROUP_X_BITMASK (MAKE_MASK(DEFAULT_GROUP_X_LOGSZ) << DEFAULT_GROUP_X_BITIDX)

#define DEFAULT_GROUP_Y_LOGSZ 5
#define DEFAULT_GROUP_Y_BITIDX (DEFAULT_GROUP_X_BITIDX + DEFAULT_GROUP_X_LOGSZ)
#define DEFAULT_GROUP_Y_BITMASK (MAKE_MASK(DEFAULT_GROUP_Y_LOGSZ) << DEFAULT_GROUP_Y_BITIDX)

#define DEFAULT_GROUP_BITIDX (DEFAULT_GROUP_Y_BITIDX + DEFAULT_GROUP_Y_LOGSZ)
#define DEFAULT_GROUP_BITMASK (1ULL << DEFAULT_GROUP_BITIDX)

#define DEFAULT_GLOBAL_X_LOGSZ 6
#define DEFAULT_GLOBAL_X_BITIDX HB_MC_EPA_LOGSZ
#define DEFAULT_GLOBAL_X_BITMASK (MAKE_MASK(DEFAULT_GLOBAL_X_LOGSZ) << DEFAULT_GLOBAL_X_BITIDX)

#define DEFAULT_GLOBAL_Y_LOGSZ 6
#define DEFAULT_GLOBAL_Y_BITIDX (DEFAULT_GLOBAL_X_BITIDX + DEFAULT_GLOBAL_X_LOGSZ)
#define DEFAULT_GLOBAL_Y_BITMASK (MAKE_MASK(DEFAULT_GLOBAL_Y_LOGSZ) << DEFAULT_GLOBAL_Y_BITIDX)

#define DEFAULT_GLOBAL_BITIDX (DEFAULT_GLOBAL_Y_BITIDX + DEFAULT_GLOBAL_Y_LOGSZ)
#define DEFAULT_GLOBAL_BITMASK (1ULL << DEFAULT_GLOBAL_BITIDX)

#define DEFAULT_DRAM_BITIDX (DEFAULT_GLOBAL_BITIDX + 1)
#define DEFAULT_DRAM_BITMASK (1ULL << DEFAULT_DRAM_BITIDX)

/**
 * Determines if an EVA is a tile-local EVA
 * @return true if EVA addresses tile-local memory, false otherwise
 */
static bool default_eva_is_local(const hb_mc_eva_t *eva)
{
	/* A LOCAL EVA is indicated by all non-EPA high-order bits set to 0 */
	return !(hb_mc_eva_addr(eva) & ~(MAKE_MASK(HB_MC_EPA_LOGSZ)));
}

/**
 * Returns the EPA and number of contiguous bytes for an EVA in a tile,
 * regardless of the continuity of the underlying NPA.
 * @param[in]  eva    An Endpoint Virtual Address
 * @param[out] epa    An Endpoint Physical Address to be set by translating #eva
 * @param[out] sz     Number of contiguous bytes remaining in the #eva segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_epa_tile(
	const hb_mc_eva_t *eva,
	hb_mc_epa_t *epa,
	size_t *sz)
{
	hb_mc_eva_t eva_masked, eva_dmem;

	eva_masked = hb_mc_eva_addr(eva) & MAKE_MASK(HB_MC_EPA_LOGSZ);
	eva_dmem = eva_masked - HB_MC_TILE_EVA_DMEM_BASE;

	if(eva_dmem < HB_MC_TILE_DMEM_SIZE){
		*epa = eva_dmem + HB_MC_TILE_EPA_DMEM_BASE;
		*sz = HB_MC_TILE_DMEM_SIZE - eva_dmem;
	}else if(eva_masked == HB_MC_TILE_EPA_CSR_FREEZE){
		*epa = eva_masked;
		*sz = sizeof(uint32_t);
	}else if(eva_masked == HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X){
		*epa = eva_masked;
		*sz = sizeof(uint32_t);
	}else if(eva_masked == HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y){
		*epa = eva_masked;
		*sz = sizeof(uint32_t);
	} else {
		bsg_pr_err("%s: Invalid EVA Address 0x%08" PRIx32 ". Does not map to an"
			" addressible tile memory locatiion.\n",
			__func__, hb_mc_eva_addr(eva));
		*epa = 0;
		*sz = 0;
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}


/**
 * Converts a local Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_local(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *o,
				const hb_mc_coordinate_t *src,
				const hb_mc_eva_t *eva,
				hb_mc_npa_t *npa, size_t *sz)
{
	int rc;
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;

	x = hb_mc_coordinate_get_x(*src);
	y = hb_mc_coordinate_get_y(*src);

	rc = default_eva_to_epa_tile(eva, &epa, sz);
	if (rc != HB_MC_SUCCESS)
		return rc;
	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

	bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
		__func__, hb_mc_eva_addr(eva),
		hb_mc_coordinate_get_x(*src),
		hb_mc_coordinate_get_y(*src),
		hb_mc_npa_get_x(npa),
		hb_mc_npa_get_y(npa),
		hb_mc_npa_get_epa(npa));
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
 * Converts a group Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_group(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *o,
				const hb_mc_coordinate_t *src,
				const hb_mc_eva_t *eva,
				hb_mc_npa_t *npa, size_t *sz)
{
	int rc;
	hb_mc_dimension_t dim;
	hb_mc_idx_t x, y, ox, oy, dim_x, dim_y;
	hb_mc_epa_t epa;

	dim = hb_mc_config_get_dimension(cfg);
	dim_x = hb_mc_dimension_get_x(dim);
	dim_y = hb_mc_dimension_get_y(dim);
	ox = hb_mc_coordinate_get_x(*o);
	oy = hb_mc_coordinate_get_y(*o);
	x = ((hb_mc_eva_addr(eva) & DEFAULT_GROUP_X_BITMASK) >> DEFAULT_GROUP_X_BITIDX);
	x += ox;
	y = ((hb_mc_eva_addr(eva) & DEFAULT_GROUP_Y_BITMASK) >> DEFAULT_GROUP_Y_BITIDX);
	y += oy;
	if(dim_x < x){
		bsg_pr_err("%s: Invalid Group EVA. X coordinate destination %d"
			"is larger than current manycore configuration\n",
			__func__, x);
		return HB_MC_FAIL;
	}

	if(dim_y < y){
		bsg_pr_err("%s: Invalid Group EVA. Y coordinate destination %d"
			"is larger than current manycore configuration\n",
			__func__, y);
		return HB_MC_FAIL;
	}

	rc = default_eva_to_epa_tile(eva, &epa, sz);
	if (rc != HB_MC_SUCCESS)
		return rc;
	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

	bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
		__func__, hb_mc_eva_addr(eva),
		hb_mc_coordinate_get_x(*src),
		hb_mc_coordinate_get_y(*src),
		hb_mc_npa_get_x(npa),
		hb_mc_npa_get_y(npa),
		hb_mc_npa_get_epa(npa));

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
 * Converts a global Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_global(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *o,
				const hb_mc_coordinate_t *src,
				const hb_mc_eva_t *eva,
				hb_mc_npa_t *npa, size_t *sz)
{
	int rc;
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;


	x = ((hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_X_BITMASK) >> DEFAULT_GLOBAL_X_BITIDX);
	y = ((hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_Y_BITMASK) >> DEFAULT_GLOBAL_Y_BITIDX);
	rc = default_eva_to_epa_tile(eva, &epa, sz);
	if (rc != HB_MC_SUCCESS)
		return rc;
	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

	bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
		__func__, hb_mc_eva_addr(eva),
		hb_mc_coordinate_get_x(*src),
		hb_mc_coordinate_get_y(*src),
		hb_mc_npa_get_x(npa),
		hb_mc_npa_get_y(npa),
		hb_mc_npa_get_epa(npa));

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

static uint32_t default_get_x_dimlog(const hb_mc_config_t *cfg)
{
	hb_mc_dimension_t dim = hb_mc_config_get_dimension(cfg);
	return ceil(log2(hb_mc_dimension_get_x(dim)));
}

static uint32_t default_get_dram_x_bitidx(const hb_mc_config_t *cfg)
{
	uint32_t xdimlog;
	// The number of bits used for the x index is determined by clog2 of the
	// x dimension (or the number of bits needed to represent the maximum x
	// dimension).
	xdimlog = default_get_x_dimlog(cfg);
	return MAKE_MASK(xdimlog);
}

static uint32_t default_get_dram_x_shift(const hb_mc_config_t *cfg)
{
	uint32_t xdimlog;
        // The location of the x index in the eva is determined by the x
	// dimension of the network, and uses the high-order bits after the
	// High-Order bit that indicates a DRAM access.
	xdimlog = default_get_x_dimlog(cfg);
	return DEFAULT_DRAM_BITIDX - xdimlog;
}

/**
 * Converts a DRAM Endpoint Virtual Address to a Network Physical Address and
 * size (contiguous bytes following the specified EVA)
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_dram(const hb_mc_config_t *cfg,
				const hb_mc_coordinate_t *o,
				const hb_mc_coordinate_t *src,
				const hb_mc_eva_t *eva,
				hb_mc_npa_t *npa, size_t *sz)
{
	uint32_t xmask, xdimlog, addrbits, shift, errmask;
	size_t maxsz;
	hb_mc_idx_t x, y;
	hb_mc_epa_t epa;
	hb_mc_dimension_t dim;

	dim = hb_mc_config_get_dimension(cfg);

	xdimlog = default_get_x_dimlog(cfg);
	xmask   = default_get_dram_x_bitidx(cfg);
	shift   = default_get_dram_x_shift(cfg);

	x = (hb_mc_eva_addr(eva) >> shift) & xmask;
	y = hb_mc_dimension_get_y(dim) + hb_mc_config_get_vcore_base_y(cfg);

	// The EVA size is determined y the address bits of the manycore
	// network. The high-order bit of the network address is reserved for
	// addressing tags in the victim cache and is not accessible to the EVA,
	// so we subtract 1. The network address is a byte address, so we add
	// two to make it a byte address.
	addrbits = hb_mc_config_get_network_bitwidth_addr(cfg) - 1 +
		log2(sizeof(uint32_t));
	maxsz = 1 << addrbits;

	// The EPA portion of an EVA is technically determined by addrbits
	// above.  However, this creates undefined behavior when (addrbits + 1 +
	// xdimlog) != DEFAULT_DRAM_BITIDX, since there are unused bits between
	// the x index and EPA.  To avoid really awful debugging, we check this
	// situation.
	errmask = MAKE_MASK(DEFAULT_DRAM_BITIDX - xdimlog);

	epa = (hb_mc_eva_addr(eva) & errmask);

	if (epa >= maxsz){
		bsg_pr_err("%s: Translation of EVA 0x%08" PRIx32 " failed. EPA requested is "
			"outside of addressible range in DRAM.");
		return HB_MC_INVALID;
	}

	*sz = maxsz - epa;
	*npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

	bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
		__func__, hb_mc_eva_addr(eva),
		hb_mc_coordinate_get_x(*src),
		hb_mc_coordinate_get_y(*src),
		hb_mc_npa_get_x(npa),
		hb_mc_npa_get_y(npa),
		hb_mc_npa_get_epa(npa));

	return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address in a source tile's address space
 * to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  priv   Private data used for this EVA Map
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int default_eva_to_npa(const hb_mc_config_t *cfg,
		const void *priv,
		const hb_mc_coordinate_t *src,
		const hb_mc_eva_t *eva,
		hb_mc_npa_t *npa, size_t *sz)
{
	const hb_mc_coordinate_t *origin;

	origin = (const hb_mc_coordinate_t *) priv;

	if(default_eva_is_dram(eva))
		return default_eva_to_npa_dram(cfg, origin, src, eva, npa, sz);
	if(default_eva_is_global(eva))
		return default_eva_to_npa_global(cfg, origin, src, eva, npa, sz);
	if(default_eva_is_group(eva))
		return default_eva_to_npa_group(cfg, origin, src, eva, npa, sz);
	if(default_eva_is_local(eva))
		return default_eva_to_npa_local(cfg, origin, src, eva, npa, sz);

	bsg_pr_err("%s: EVA 0x%08" PRIx32 " did not map to a known region\n",
		hb_mc_eva_addr(eva), __func__);
	return HB_MC_FAIL;
}

/**
 * Check if a DRAM EPA is valid.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the EPA is valid, false otherwise.
 */
static bool default_dram_epa_is_valid(const hb_mc_config_t *cfg,
                                      hb_mc_epa_t epa,
                                      const hb_mc_coordinate_t *tgt)
{
        return epa < hb_mc_config_get_dram_size(cfg);
}


/**
 * Check if a local EPA is valid.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the EPA is valid, false otherwise.
 */
static bool default_local_epa_is_valid(const hb_mc_config_t *config,
                                       hb_mc_epa_t epa,
                                       const hb_mc_coordinate_t *tgt)
{
        hb_mc_epa_t floor = HB_MC_TILE_EPA_DMEM_BASE;
        hb_mc_epa_t ceil  = HB_MC_TILE_EPA_DMEM_BASE + hb_mc_config_get_dmem_size(config);
        return (epa >= floor) && (epa < ceil);
}

/**
 * Check if an NPA is a host DRAM.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is DRAM, false otherwise.
 */
static bool default_npa_is_dram(const hb_mc_config_t *config,
                                const hb_mc_npa_t *npa,
                                const hb_mc_coordinate_t *tgt)
{
	char npa_str[64];
	bool is_dram = (hb_mc_npa_get_y(npa) == hb_mc_config_get_dram_y(config))
                && default_dram_epa_is_valid(config, hb_mc_npa_get_epa(npa), tgt);

	bsg_pr_dbg("%s: npa %s %s DRAM\n",
		   __func__,
		   hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)),
		   (is_dram ? "is" : "is not"));

        return is_dram;
}

/**
 * Check if an NPA is a host address.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is host, false otherwise.
 */
static bool default_npa_is_host(const hb_mc_config_t *config,
                                const hb_mc_npa_t *npa,
                                const hb_mc_coordinate_t *tgt)
{
	char npa_str[64];
        hb_mc_coordinate_t host = hb_mc_config_get_host_interface(config);
	bool is_host = hb_mc_coordinate_get_x(host) == hb_mc_npa_get_x(npa) &&
                hb_mc_coordinate_get_y(host) == hb_mc_npa_get_y(npa);

	bsg_pr_dbg("%s: npa %s %s a host address\n",
		   __func__,
		   hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)),
		   (is_host ? "is" : "is not"));

        // does your coordinate map to the host?
        // I guess we're generally permissive with host EPAs
	return is_host;
}

/**
 * Check if an NPA is a local address.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is local, false otherwise.
 */
static bool default_npa_is_local(const hb_mc_config_t *config,
                                 const hb_mc_npa_t *npa,
                                 const hb_mc_coordinate_t *tgt)
{
        // does your coordinate map to this tgt v-core and is your epa valid?
        return (hb_mc_npa_get_x(npa) == hb_mc_coordinate_get_x(*tgt)) &&
                (hb_mc_npa_get_y(npa) == hb_mc_coordinate_get_y(*tgt)) &&
                default_local_epa_is_valid(config, hb_mc_npa_get_epa(npa), tgt);
}

/**
 * Check if an NPA is a global address.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is global, false otherwise.
 */
static bool default_npa_is_global(const hb_mc_config_t *config,
                                  const hb_mc_npa_t *npa,
                                  const hb_mc_coordinate_t *tgt)
{
        hb_mc_idx_t base_x = 0;
        hb_mc_idx_t base_y = 0;
        hb_mc_idx_t ceil_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension(config));
        hb_mc_idx_t ceil_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension(config));

        // does your coordinate map to any v-core and is your epa valid?
        return (hb_mc_npa_get_x(npa) >= base_x) && (hb_mc_npa_get_x(npa) <= ceil_x) &&
                (hb_mc_npa_get_y(npa) >= base_y) && (hb_mc_npa_get_y(npa) <= ceil_y) &&
                default_local_epa_is_valid(config, hb_mc_npa_get_epa(npa), tgt);

}

/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_dram(const hb_mc_config_t *cfg,
                                   const hb_mc_coordinate_t *origin,
                                   const hb_mc_coordinate_t *tgt,
                                   const hb_mc_npa_t *npa,
                                   hb_mc_eva_t *eva,
                                   size_t *sz)
{
        // build the eva
        hb_mc_eva_t addr = 0;
	hb_mc_eva_t xshift = default_get_dram_x_shift(cfg);

        addr |= hb_mc_npa_get_epa(npa); // set the byte address
        addr |= hb_mc_npa_get_x(npa) << xshift; // set the x coordinate
        addr |= 1 << DEFAULT_DRAM_BITIDX; // set the DRAM bit
        *eva  = addr;

        // this is lame but we are basically saying "you can write to this word only"
        *sz = 4 - (hb_mc_npa_get_epa(npa) & 0x3);

        // done
        return HB_MC_SUCCESS;
}

/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_global_remote(const hb_mc_config_t *cfg,
                                            const hb_mc_coordinate_t *origin,
                                            const hb_mc_coordinate_t *tgt,
                                            const hb_mc_npa_t *npa,
                                            hb_mc_eva_t *eva,
                                            size_t *sz)
{
        // build the eva
        hb_mc_eva_t addr = 0;
        addr |= hb_mc_npa_get_epa(npa); // set the byte address
        addr |= hb_mc_npa_get_x(npa) << DEFAULT_GLOBAL_X_BITIDX; // set x coordinate
        addr |= hb_mc_npa_get_y(npa) << DEFAULT_GLOBAL_Y_BITIDX; // set y coordinate
        addr |= 1 << DEFAULT_GLOBAL_BITIDX; // set the global bit

	*eva = addr;

        // this is lame but we are basically saying "you can write to this word only"
        *sz = 4 - (hb_mc_npa_get_epa(npa) & 0x3);

        // done
        return HB_MC_SUCCESS;
}

//////////////////////////////////////////////////////////////////
// At the moment we treat host, local, and globals all the same //
//////////////////////////////////////////////////////////////////

/**
 * Translate an NPA for the host interface to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_host(const hb_mc_config_t *cfg,
                                   const hb_mc_coordinate_t *origin,
                                   const hb_mc_coordinate_t *tgt,
                                   const hb_mc_npa_t *npa,
                                   hb_mc_eva_t *eva,
                                   size_t *sz)
{
        return default_npa_to_eva_global_remote(cfg, origin, tgt, npa, eva, sz);
}

/**
 * Translate an local NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_local(const hb_mc_config_t *cfg,
                                    const hb_mc_coordinate_t *origin,
                                    const hb_mc_coordinate_t *tgt,
                                    const hb_mc_npa_t *npa,
                                    hb_mc_eva_t *eva,
                                    size_t *sz)
{
        return default_npa_to_eva_global_remote(cfg, origin, tgt, npa, eva, sz);
}

/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_global(const hb_mc_config_t *cfg,
                                     const hb_mc_coordinate_t *origin,
                                     const hb_mc_coordinate_t *tgt,
                                     const hb_mc_npa_t *npa,
                                     hb_mc_eva_t *eva,
                                     size_t *sz)
{
        return default_npa_to_eva_global_remote(cfg, origin, tgt, npa, eva, sz);
}

/**
 * Translate a Network Physical Address to an Endpoint Virtual Address in a
 * target tile's address space
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  priv   Private data used for this EVA Map
 * @param[in]  tgt    Coordinates of the target tile
 * @param[in]  len    Number of tiles in the target tile's group
 * @param[in]  npa    An npa to translate
 * @param[out] eva    An eva to set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int default_npa_to_eva(const hb_mc_config_t *cfg,
		const void *priv,
		const hb_mc_coordinate_t *tgt,
		const hb_mc_npa_t *npa,
		hb_mc_eva_t *eva, size_t *sz)
{
	const hb_mc_coordinate_t *origin = (const hb_mc_coordinate_t*)priv;

        if(default_npa_is_dram(cfg, npa, tgt))
                return default_npa_to_eva_dram(cfg, origin, tgt, npa, eva, sz);

        if(default_npa_is_host(cfg, npa, tgt))
                return default_npa_to_eva_host(cfg, origin, tgt, npa, eva, sz);

        if(default_npa_is_local(cfg, npa, tgt))
                return default_npa_to_eva_local(cfg, origin, tgt, npa, eva, sz);

        if(default_npa_is_global(cfg, npa, tgt))
                return default_npa_to_eva_global(cfg, origin, tgt, npa, eva, sz);

	return HB_MC_FAIL;
}

/**
 * Returns the number of contiguous bytes following an EVA, regardless of
 * the continuity of the underlying NPA.
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  priv   Private data used for this EVA Map
 * @param[in]  eva    An eva
 * @param[out] sz     Number of contiguous bytes remaining in the #eva segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_size(
	const hb_mc_config_t *cfg,
	const void *priv,
	const hb_mc_eva_t *eva,
	size_t *sz)
{
	hb_mc_npa_t npa;
	hb_mc_epa_t epa;
	const hb_mc_coordinate_t *o;

	o = (const hb_mc_coordinate_t *) priv;

	if(default_eva_is_dram(eva))
		return default_eva_to_npa_dram(cfg, o, o, eva, &npa, sz);
	if(default_eva_is_global(eva))
		return default_eva_to_npa_global(cfg, o, o, eva, &npa, sz);
	if(default_eva_is_group(eva))
		return default_eva_to_npa_group(cfg, o, o, eva, &npa, sz);
	if(default_eva_is_local(eva))
		return default_eva_to_npa_local(cfg, o, o, eva, &npa, sz);

	bsg_pr_err("%s: EVA 0x%08" PRIx32 " did not map to a known region\n",
		hb_mc_eva_addr(eva), __func__);
	return HB_MC_FAIL;

}

const hb_mc_coordinate_t default_origin = {.x = HB_MC_CONFIG_VCORE_BASE_X,
					   .y = HB_MC_CONFIG_VCORE_BASE_Y};
hb_mc_eva_map_t default_map = {
	.eva_map_name = "Default EVA space",
	.priv = (const void *)(&default_origin),
	.eva_to_npa = default_eva_to_npa,
	.eva_size = default_eva_size,
	.npa_to_eva  = default_npa_to_eva,
};

/**
 * Translate a Network Physical Address to an Endpoint Virtual Address in a
 * target tile's address space
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinates of the target tile
 * @param[in]  npa    An npa to translate
 * @param[out] eva    An eva to set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_npa_to_eva(const hb_mc_config_t *cfg,
		const hb_mc_eva_map_t *map,
		const hb_mc_coordinate_t *tgt,
		const hb_mc_npa_t *npa,
		hb_mc_eva_t *eva, size_t *sz)
{
	int err;

	err = map->npa_to_eva(cfg, map->priv, tgt, npa, eva, sz);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address in a source tile's address space
 * to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by using #map to translate #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_eva_to_npa(const hb_mc_config_t *cfg,
		const hb_mc_eva_map_t *map,
		const hb_mc_coordinate_t *src,
		const hb_mc_eva_t *eva,
		hb_mc_npa_t *npa, size_t *sz)
{
	int err;

	err = map->eva_to_npa(cfg, map->priv, src, eva, npa, sz);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address in a source tile's address space
 * to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  eva    An eva to translate
 * @param[out] sz     Number of contiguous bytes remaining in the #eva segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_eva_size(const hb_mc_config_t *cfg,
		const hb_mc_eva_map_t *map,
		const hb_mc_eva_t *eva, size_t *sz)
{
	int err;

	err = map->eva_size(cfg, map->priv, eva, sz);
	if (err != HB_MC_SUCCESS)
		return err;

	return HB_MC_SUCCESS;
}


static size_t min_size_t(size_t x, size_t y)
{
	return x < y ? x : y;
}

/**
 * Write memory out to manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_write(hb_mc_manycore_t *mc,
			const hb_mc_eva_map_t *map,
			const hb_mc_coordinate_t *tgt,
			const hb_mc_eva_t *eva,
			const void *data, size_t sz)
{
	int err;
	const hb_mc_config_t* config;
	size_t dest_sz, xfer_sz;
	hb_mc_npa_t dest_npa;
	char *destp;
	config = hb_mc_manycore_get_config(mc);

	err = hb_mc_eva_size(config, map, eva, &dest_sz);
	if (sz > dest_sz){
		bsg_pr_err("%s: Error, requested copy to region that is smaller "
			"than buffer\n", __func__);
		return HB_MC_FAIL;
	}

	destp = (char *)data;
	while(sz > 0){
		err = hb_mc_eva_to_npa(config, map, tgt, eva, &dest_npa, &dest_sz);
		if(err != HB_MC_SUCCESS){
			bsg_pr_err("%s: Failed to translate EVA into a NPA\n",
				__func__);
			return err;
		}
		xfer_sz = min_size_t(sz, dest_sz);

		err = hb_mc_manycore_write_mem(mc, &dest_npa, destp, xfer_sz);
		if(err != HB_MC_SUCCESS){
			bsg_pr_err("%s: Failed to copy data from host to NPA\n",
				__func__);
			return err;
		}

		destp += xfer_sz;
		sz -= xfer_sz;
		eva += xfer_sz;
	}

	return HB_MC_SUCCESS;
}

/**
 * Read memory from manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa map
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from the manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_read(hb_mc_manycore_t *mc,
			const hb_mc_eva_map_t *map,
			const hb_mc_coordinate_t *tgt,
			const hb_mc_eva_t *eva,
			void *data, size_t sz)
{
	int err;
	const hb_mc_config_t* config;
	size_t src_sz, xfer_sz;
	hb_mc_npa_t src_npa;
	char *srcp;

	config = hb_mc_manycore_get_config(mc);

	err = hb_mc_eva_size(config, map, eva, &src_sz);
	if (sz > src_sz){
		bsg_pr_err("%s: Error, requested read from region that is smaller "
			"than buffer\n", __func__);
		return HB_MC_FAIL;
	}

	srcp = (char *)data;
	while(sz > 0){
		err = hb_mc_eva_to_npa(config, map, tgt, eva, &src_npa, &src_sz);
		if(err != HB_MC_SUCCESS){
			bsg_pr_err("%s: Failed to translate EVA into a NPA\n",
				__func__);
			return err;
		}

		xfer_sz = min_size_t(sz, src_sz);

		err = hb_mc_manycore_read_mem(mc, &src_npa, srcp, xfer_sz);
		if(err != HB_MC_SUCCESS){
			bsg_pr_err("%s: Failed to copy data from host to NPA\n",
				__func__);
			return err;
		}

		srcp += xfer_sz;
		sz -= xfer_sz;
		eva += xfer_sz;
	}
	return HB_MC_SUCCESS;
}

/**
 * Set a EVA memory region to a value
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[in]  val    The value to write to the region
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_memset(hb_mc_manycore_t *mc,
			const hb_mc_eva_map_t *map,
			const hb_mc_coordinate_t *tgt,
			const hb_mc_eva_t *eva,
			uint8_t val, size_t sz)
{
	int err;
	const hb_mc_config_t* config;
	size_t dest_sz, xfer_sz;
	hb_mc_npa_t dest_npa;
	config = hb_mc_manycore_get_config(mc);

	err = hb_mc_eva_size(config, map, eva, &dest_sz);
	if (sz > dest_sz){
		bsg_pr_err("%s: Error, requested copy to region that is smaller "
			"than buffer\n", __func__);
		return HB_MC_FAIL;
	}

	while(sz > 0){
		err = hb_mc_eva_to_npa(config, map, tgt, eva, &dest_npa, &dest_sz);
		if(err != HB_MC_SUCCESS){
			bsg_pr_err("%s: Failed to translate EVA into a NPA\n",
				__func__);
			return err;
		}
		xfer_sz = min_size_t(sz, dest_sz);

		err = hb_mc_manycore_memset(mc, &dest_npa, val, xfer_sz);
		if(err != HB_MC_SUCCESS){
			bsg_pr_err("%s: Failed to set NPA region to value\n",
				__func__);
			return err;
		}

		sz -= xfer_sz;
		eva += xfer_sz;
	}

	return HB_MC_SUCCESS;
}
