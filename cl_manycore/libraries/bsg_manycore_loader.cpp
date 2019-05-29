#define DEBUG
#include <bsg_manycore_loader.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_vcache.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_npa.h>

#include <cinttypes>
#include <elf.h>
#include <endian.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __cplusplus
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>
#include <cstdbool>
#else
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#endif

static size_t min_size_t(size_t x, size_t y)
{
	return x < y ? x : y;
}

/////////////////////////////////
// Accessors for the ELF types //
/////////////////////////////////

#define EM_RISCV 	243 	/* RISC-V */

static Elf32_Word RV32_Word_to_host(Elf32_Word word)
{
	return le32toh(word);
}

static Elf32_Half RV32_Half_to_host(Elf32_Half half)
{
	return le16toh(half);
}

static Elf32_Sword RV32_Sword_to_host(Elf32_Sword sword)
{
	return le32toh(sword);
}

static Elf32_Xword RV32_Xword_to_host(Elf32_Xword xword)
{
	return le64toh(xword);
}

static Elf32_Sxword RV32_Sxword_to_host(Elf32_Sxword sxword)
{
	return le64toh(sxword);
}

static Elf32_Addr RV32_Addr_to_host(Elf32_Addr addr)
{
	return le32toh(addr);
}

static Elf32_Off RV32_Off_to_host(Elf32_Off off)
{
	return le32toh(off);
}

static Elf32_Section RV32_Section_to_host(Elf32_Section section)
{
	return le16toh(section);
}

static Elf32_Versym RV32_Versym_to_host(Elf32_Versym versym)
{
	return RV32_Half_to_host(versym);
}


/**
 * Get the max memory capacity of a program segment (these map to hardware).
 * @param[in] mc       A manycore instance.
 * @param[in] id       An EVA id.
 * @param[in] phdr     A program header for the segment.
 * @param[in] tile     A manycore coordinate.
 * @return the max size for #segment.
 */
static size_t hb_mc_loader_get_tile_segment_capacity(hb_mc_manycore_t *mc,
						     const hb_mc_eva_map_t *id,
						     const Elf32_Phdr *phdr,
						     hb_mc_coordinate_t tile)
{
	/*
	  The right thing to do here would be to get the hardware component
	  from the NPA, and then check the capacity of that.

	  We don't currently have a function that maps NPAs to their
	  underlying hardware type.

	  So for now we just check the top bit of the EVA.
	  If the top bit is 1, then address is DRAM (
	*/

	/* if the EVA maps to DRAM */
	if (RV32_Addr_to_host(phdr->p_vaddr) & (1<<31)) {
		return hb_mc_manycore_get_size_dram(mc);
	} else {
		return hb_mc_tile_get_size_dmem(mc, &tile);
	}
}

/**
 * Format a segment program header as a human readable string.
 * @param[in] phdr    A program header.
 * @param[in] buffer  A buffer to which string data is written.
 * @param[in] n       The size of #buffer.
 * @return A pointer to #buffer.
 */
static const char *hb_mc_loader_segment_to_string(const Elf32_Phdr *phdr,
						  char *buffer,
						  size_t n)
{
	snprintf(buffer, n, "segment @ 0x%08" PRIx32 "",
		 RV32_Addr_to_host(phdr->p_vaddr));
	return buffer;
}

/**
 * Writes program data to an EVA.
 * @param[in] phdr       The program header for this data (for debugging).
 * @param[in] data       Data to be written out.
 * @param[in] start_eva  The start EVA.
 * @param[in] mc         A manycore instance.
 * @param[in] map        And EVA to NPA map
 * @param[in] tile       A manycore coordinate.
 * @return HB_MC_SUCCESS if succesful. Otherwise and error code is returned.
 */
static int hb_mc_loader_eva_write(const Elf32_Phdr *phdr,
				     const unsigned char *data, size_t sz,
				     hb_mc_eva_t start_eva,
				     hb_mc_manycore_t *mc,
				     const hb_mc_eva_map_t *map,
				     hb_mc_coordinate_t tile)
{
	hb_mc_eva_t eva = start_eva;
	size_t off = 0, rem = sz;
	int rc;
	char segname[64];

	hb_mc_loader_segment_to_string(phdr, segname, sizeof(segname));

	rc = hb_mc_manycore_eva_write(mc, map, &tile, &start_eva, data, sz);
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to write %s @ eva 0x%08x for tile (%d, %d)"
			": %s\n",
			__func__,
			segname,
			eva,
			hb_mc_coordinate_get_x(tile),
			hb_mc_coordinate_get_y(tile),
			hb_mc_strerror(rc));
		return rc;
	}

	return HB_MC_SUCCESS;
}

/**
 * Writes program data to an EVA.
 * @param[in] phdr       The program header for this data (for debugging).
 * @param[in] val        The value to be written
 * @param[in] start_eva  The start EVA.
 * @param[in] mc         A manycore instance.
 * @param[in] map        And EVA to NPA map
 * @param[in] tile       A manycore coordinate.
 * @return HB_MC_SUCCESS if succesful. Otherwise and error code is returned.
 */
static int hb_mc_loader_eva_memset(const Elf32_Phdr *phdr,
				uint8_t val, size_t sz,
				hb_mc_eva_t start_eva,
				hb_mc_manycore_t *mc,
				const hb_mc_eva_map_t *map,
				hb_mc_coordinate_t tile)
{
	hb_mc_eva_t eva = start_eva;
	size_t off = 0, rem = sz;
	int rc;
	char segname[64];

	hb_mc_loader_segment_to_string(phdr, segname, sizeof(segname));

	rc = hb_mc_manycore_eva_memset(mc, map, &tile, &start_eva, val, sz);
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to memset %s @ eva 0x%08x for tile (%d, %d)"
			": %s\n",
			__func__,
			segname,
			eva,
			hb_mc_coordinate_get_x(tile),
			hb_mc_coordinate_get_y(tile),
			hb_mc_strerror(rc));
		return rc;
	}

	return HB_MC_SUCCESS;
}

/**
 * Load a program segment.
 * @param[in] mc       A manycore instance.
 * @param[in] map      A EVA to NPA map.
 * @param[in] phdr     A program header for the data to be loaded.
 * @param[in] segdata  Program data to be loaded.
 * @param[in] tile     A manycore coordinate.
 * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
 */
static int hb_mc_loader_load_tile_segment(hb_mc_manycore_t *mc,
					  const hb_mc_eva_map_t *map,
					  const Elf32_Phdr *phdr,
					  const unsigned char *segdata,
					  hb_mc_coordinate_t tile)
{
	int rc;
	size_t cap, seg_sz;
	char segname[64];

	hb_mc_loader_segment_to_string(phdr, segname, sizeof(segname));

	/* get hardware capacity of the segment */
	cap = hb_mc_loader_get_tile_segment_capacity(mc, map, phdr, tile);
	seg_sz = RV32_Word_to_host(phdr->p_memsz);

        /* return error if the hardware lacks the capacity */
	if (cap < seg_sz) {
		bsg_pr_err("%s: '%s' (%zu bytes) exceeds "
			   "maximum (%zu bytes)\n",
			   __func__,
			   segname,
			   seg_sz,
			   cap);
		return HB_MC_FAIL;
	}

	/* load initialized data */
	hb_mc_eva_t eva = RV32_Addr_to_host(phdr->p_vaddr); /* get the eva */
	size_t file_sz = RV32_Word_to_host(phdr->p_filesz); /* get the size of segdata */

	rc = hb_mc_loader_eva_write(phdr, segdata, file_sz, eva, mc, map, tile);
	if (rc != HB_MC_SUCCESS)
		return rc;

	/* load zeroed data */
	size_t zeros_sz  = seg_sz - file_sz;  // zeros are the remainder of the segment
	eva += file_sz; // increment eva by number of initialized bytes written

	rc = hb_mc_loader_eva_memset(phdr, 0, zeros_sz, eva, mc, map, tile);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_SUCCESS;
}

/**
 * Load a program segment.
 * @param[in] mc       A manycore instance.
 * @param[in] map      A EVA to NPA map.
 * @param[in] phdr     A program header for the data to be loaded.
 * @param[in] segdata  Program data to be loaded.
 * @param[in] tile     A manycore coordinate.
 * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
 */
static int hb_mc_loader_load_tiles_segment(hb_mc_manycore_t *mc,
					   const hb_mc_eva_map_t *map,
					   const Elf32_Phdr *phdr,
					   const unsigned char *segdata,
					   const hb_mc_coordinate_t *tiles,
					   uint32_t ntiles)
{
	int rc;

	for (uint32_t i = 0; i < ntiles; i++) {
		rc = hb_mc_loader_load_tile_segment(mc, map, phdr, segdata, tiles[i]);
		if (rc != HB_MC_SUCCESS)
			return rc;
	}
	return HB_MC_SUCCESS;
}

/**
 * Load a tile's ICACHE.
 * @param[in] mc       A manycore instance.
 * @param[in] phdr     The program header to be loaded.
 * @param[in] segdata  The program data to be loaded.
 * @param[in] tile     A tile whose ICACHE needs to be initialized.
 * @return HB_MC_SUCCESS if succseful. Otherwise an error code is returned.
 */
static int hb_mc_loader_load_tile_icache(hb_mc_manycore_t *mc,
					  const hb_mc_eva_map_t *map,
					  const Elf32_Phdr *phdr,
					  const unsigned char *segdata,
					  hb_mc_coordinate_t tile)
{
	int rc;

	/* get the NPA of this tiles ICACHE */
	hb_mc_npa_t icache_npa = hb_mc_npa(tile, HB_MC_TILE_EPA_ICACHE);

        /* write min(icache size, segment size) bytes */
	size_t sz = min_size_t(RV32_Word_to_host(phdr->p_filesz),
			       hb_mc_tile_get_size_icache(mc, &tile));

	bsg_pr_dbg("%s: writing %zu bytes to (%d,%d)'s icache @ EPA 0x%08" PRIx32 "\n",
		   __func__, sz,
		   hb_mc_coordinate_get_x(tile),
		   hb_mc_coordinate_get_y(tile),
		   hb_mc_npa_get_epa(&icache_npa)
		);

	/*
	  The address space of the ICACHE is larger than the ICACHE itself.
	  Bits 12-23 actually indicate the tag data rather than a location.
	  Only bits 0-11 actually index the memory in the ICACHE.
	  It's important that bits 10-21 are zero.
	 */
	if (((hb_mc_npa_get_epa(&icache_npa)<<2) + sz) & 0x00FFF000) {
		bsg_pr_dbg("%s: Oops: ICACHE EPA sets tag bits\n", __func__);
		return HB_MC_FAIL;
	}

	rc = hb_mc_manycore_write_mem(mc, &icache_npa, segdata, sz);
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_dbg("%s: failed to write to (%d,%d)'s icache: %s\n",
			   __func__,
			   hb_mc_coordinate_get_x(tile),
			   hb_mc_coordinate_get_y(tile),
			   hb_mc_strerror(rc));
		return rc;
	}

	return HB_MC_SUCCESS;
}

/**
 * Load tiles' ICACHE.
 * @param[in] mc       A manycore instance.
 * @param[in] phdr     The program header to be loaded.
 * @param[in] segdata  The program data to be loaded.
 * @param[in] tiles    Tiles whose ICACHE needs to be initialized.
 * @param[in] ntiles   Number of tiles.
 * @return HB_MC_SUCCESS if succseful. Otherwise an error code is returned.
 */
static int hb_mc_loader_load_tiles_icache(hb_mc_manycore_t *mc,
					  const hb_mc_eva_map_t *map,
					  const Elf32_Phdr *phdr,
					  const unsigned char *segdata,
					  const hb_mc_coordinate_t *tiles,
					  uint32_t ntiles)
{	int rc;

	for (uint32_t i = 0; i < ntiles; i++) {
		rc = hb_mc_loader_load_tile_icache(mc, map, phdr, segdata, tiles[i]);
		if (rc != HB_MC_SUCCESS)
			return rc;
	}
	return HB_MC_SUCCESS;
}

/**
 * Validate that a buffer contains a valid ELF format
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_elf_validate(const void *elf, size_t sz)
{
    	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)elf;

	/* sz should definitely be larger than the elf header */
	if (sz < sizeof(*ehdr)) {
		bsg_pr_dbg("%s: 'sz' = %zu is less than minimum valid object size\n",
			   __func__, sz);
		return HB_MC_INVALID;
	}

	/* validate magic number */
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
		bsg_pr_dbg("%s: object is missing the elf magic number\n",
			   __func__);
		return HB_MC_INVALID;
	}

	/* check that this object is for a 32-bit machine */
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		bsg_pr_dbg("%s: object is not for 32-bit machine\n",
			   __func__);
		return HB_MC_INVALID;
	}

	/* check that this object is little-endian */
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		bsg_pr_dbg("%s: object is not 2's complement, little endian\n",
			   __func__);
		return HB_MC_INVALID;
	}

	/* check that this object is executable */
	if (RV32_Half_to_host(ehdr->e_type) != ET_EXEC) {
		bsg_pr_dbg("%s: object is not executable\n",
			   __func__);
		return HB_MC_INVALID;
	}

	/* check that this machine is RV32 */
	if (RV32_Half_to_host(ehdr->e_machine) != EM_RISCV) {
		bsg_pr_dbg("%s: object does not target RISC-V\n",
			   __func__);
		return HB_MC_INVALID;
	}

	/* make some size checks? */

	/* at this point the caller is trying pretty hard to mess up*/
	return HB_MC_SUCCESS;
}


/**
 * Check is a segment should not be loaded.
 * This generally pertains to the program data the .data section (goes to DMEM).
 * @param[in] mc      A manycore instance.
 * @param[in] phdr    A program header.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tiles   Tiles being loaded.
 * @param[in] ntiles  Number of tiles being loaded.
 * @return true if the segment should only be loaded once.
 */
static bool hb_mc_loader_segment_is_load_never(hb_mc_manycore *mc,
					       const Elf32_Phdr *phdr,
					       const hb_mc_eva_map_t *map,
					       const hb_mc_coordinate_t *tiles,
					       uint32_t ntiles)
{
	switch (RV32_Word_to_host(phdr->p_type)) {
	case PT_LOAD:
		return false;
	default:
		return true;
	}
}

/**
 * Check is a segment should only be loaded once.
 * This generally pertains to the program data containing .text and .dram (goes to DRAM).
 * @param[in] mc      A manycore instance.
 * @param[in] phdr    A program header.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tiles   Tiles being loaded.
 * @param[in] ntiles  Number of tiles being loaded.
 * @return true if the segment should only be loaded once.
 */
static bool hb_mc_loader_segment_is_load_once(hb_mc_manycore *mc,
					      const Elf32_Phdr *phdr,
					      const hb_mc_eva_map_t *map,
					      const hb_mc_coordinate_t *tiles,
					      uint32_t ntiles)
{
	/* the correct thing to do is get the NPA and check if it maps to DRAM */
	/* but there's no function at the moment that maps NPAs to
	   what the underlying hardware is */
	/* for now, just check that the most significant bit of the EVA is 1 */
	hb_mc_eva_t eva = RV32_Addr_to_host(phdr->p_vaddr);
	return (eva & (1<<31) ? true : false);
}

/**
 * Check is a segment should be written ICACHE.
 * @param[in] mc      A manycore instance.
 * @param[in] phdr    A program header.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tiles   Tiles being loaded.
 * @param[in] ntiles  Number of tiles being loaded.
 * @return true if the segment is should be loaded ICACHE.
 */
static bool hb_mc_loader_segment_is_load_icache(hb_mc_manycore *mc,
						const Elf32_Phdr *phdr,
						const hb_mc_eva_map_t *map,
						const hb_mc_coordinate_t *tiles,
						uint32_t ntiles)
{
	Elf32_Word type, flags;

	type  = RV32_Word_to_host(phdr->p_type);
	flags = RV32_Word_to_host(phdr->p_flags);

	return (type == PT_LOAD) && (flags & PF_X);
}

/**
 * Get a program segment from a binary object.
 * @param[in] bin       A binary object to load onto the tiles.
 * @param[in] sz        The size of the binary object.
 * @param[in] segidx    A segment index to get.
 * @param[out] ophdr    Is set to the program header for request segment.
 * @param[out] osegdata Is set to the segment data to be loaded.
 * @return HB_MC_SUCCESS if succseful. Otherwise an error code is returned.
 */
static int hb_mc_loader_get_segment(const void *bin, size_t sz, unsigned segidx,
				    const Elf32_Phdr **ophdr, const unsigned char **osegdata)
{
	const unsigned char *segdata, *data = (const unsigned char *)bin;
	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr*)bin;
	const Elf32_Phdr *phdr_table, *phdr;
	size_t phoff = RV32_Off_to_host(ehdr->e_phoff);

	/*
	   Check that the binary is big enough.

	   The binary should be at least as big as the offset to the
	   program header table + the size of the header table.
	 */
	size_t header = phoff + (RV32_Half_to_host(ehdr->e_phnum) * sizeof(*phdr));
	if (sz < header) {
		bsg_pr_dbg("%s: Header offset+size (%zu) for segment %u "
			   "exceeds object size (%zu)\n",
			   __func__, header, segidx, sz);
		return HB_MC_INVALID;
	}
	/* Get our program header */
	phdr_table = (const Elf32_Phdr*) &data[phoff];
	phdr = &phdr_table[segidx];

	size_t segoff = RV32_Off_to_host(phdr->p_offset);
	size_t segsz  = RV32_Word_to_host(phdr->p_filesz);

	/* Again, check that the binary is big enough. */
	if (sz < (segoff + segsz)) {
		bsg_pr_dbg("%s: Data offset+size (%zu) for segument %u "
			   "exceeds object size (%zu)\n",
			   __func__, segoff+segsz, segidx, sz);
		return HB_MC_INVALID;
	}

	/* Get our program data */
	segdata = &data[segoff];

	/* set outputs */
	*ophdr    = phdr;
	*osegdata = segdata;

	return HB_MC_SUCCESS;
}

/**
 * Load program segments onto tiles.
 * @param[in] bin     A binary object to load onto the tiles.
 * @param[in] sz      The size of the binary object.
 * @param[in] mc      A manycore instance.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tiles   Tiles to load.
 * @param[in] ntiles  The number of tiles to load.
 * @return HB_MC_SUCCESS if succseful. Otherwise an error code is returned.
 */
static int hb_mc_loader_load_segments(const void *bin, size_t sz,
				      hb_mc_manycore_t *mc, const hb_mc_eva_map_t *map,
				      const hb_mc_coordinate_t *tiles, uint32_t ntiles)
{
	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)bin;
	int rc, icache_segidx;

	/////////////////////////////////////
	// Load all segments to their EVAs //
	/////////////////////////////////////

	/* for each program header */
	for (int segidx = 0; segidx < RV32_Half_to_host(ehdr->e_phnum); segidx++) {
		const Elf32_Phdr *phdr;
		const unsigned char *segdata;

		rc = hb_mc_loader_get_segment(bin, sz, segidx, &phdr, &segdata);
		if (rc != HB_MC_SUCCESS)
			return rc;

		/* check if program header should be loaded never, once, or for each tile */
		if (hb_mc_loader_segment_is_load_never(mc, phdr, map, tiles, ntiles)) {
			// this segment should not be loaded
			continue;
		} else if (hb_mc_loader_segment_is_load_once(mc, phdr, map, tiles, ntiles)) {
			// this segment should be loaded only once (e.g. DRAM = .text + .dram)
			rc = hb_mc_loader_load_tile_segment(mc, map, phdr, segdata, tiles[0]);
			if (rc != HB_MC_SUCCESS)
				return rc;
		} else { // this segment should be loaded once for each tile (e.g. DMEM = .data)
			rc = hb_mc_loader_load_tiles_segment(mc, map, phdr, segdata,
							     tiles, ntiles);
			if (rc != HB_MC_SUCCESS)
				return rc;
		}

		/*
		   The first 1K words of program text needs to be written to icache
		   as well as once to DRAM.

		   So when we find the segment with program text in it, we save it
		   for further loading.
		*/
		if (hb_mc_loader_segment_is_load_icache(mc, phdr, map, tiles, ntiles))
			icache_segidx = segidx;
	}

	const Elf32_Phdr *icache_phdr;
	const unsigned char *icache_data;

	rc = hb_mc_loader_get_segment(bin, sz, icache_segidx, &icache_phdr, &icache_data);
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_dbg("%s: while fetching ICACHE segment (segidx = %d): %s\n",
			   __func__, icache_segidx, hb_mc_strerror(icache_segidx));
		return rc;
	}

	/* init icache */
	rc = hb_mc_loader_load_tiles_icache(mc, map, icache_phdr, icache_data, tiles, ntiles);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_FAIL;
}

/**
 * Perform register setup for a tile.
 * @param[in] mc         A manycore instance.
 * @param[in] map        An EVA<->NPA map.
 * @param[in] tile       A tile to setup.
 * @param[in] all_tiles  All tiles being loaded.
 * @param[in] ntiles     Number of tiles being loaded.
 * @return
 */
static int hb_mc_loader_tile_set_registers(hb_mc_manycore_t *mc,
					   const hb_mc_eva_map_t *map,
					   hb_mc_coordinate_t tile,
					   const hb_mc_coordinate_t *all_tiles,
					   uint32_t ntiles)
{
	int rc;

	/* set the origin tile */
	rc = hb_mc_tile_set_origin(mc, &tile, &all_tiles[0]); // we assume 0 is the origin

	if (rc != HB_MC_SUCCESS) {
		bsg_pr_dbg("%s: failed to write (%d,%d)'s origin registers: %s\n",
			   __func__,
			   hb_mc_coordinate_get_x(tile),
			   hb_mc_coordinate_get_y(tile),
			   hb_mc_strerror(rc));
		return rc;
	}

	return HB_MC_SUCCESS;
}

/**
 * Performance miscellaneous initialization on one tile.
 * @param[in] mc      A manycore instance.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tile    A tile to initialize
 * @param[in] tiles   The list of tiles being initialized.
 * @param[in] ntiles  The number of tiles being initialized.
 * @return HB_MC_SUCCESS if an error occured. Otherwise an error code is returned.
 */
static int hb_mc_loader_tile_initialize(hb_mc_manycore_t *mc,
					const hb_mc_eva_map_t *map,
					hb_mc_coordinate_t tile,
					const hb_mc_coordinate_t *all_tiles,
					uint32_t ntiles)
{
	int rc;

	rc = hb_mc_loader_tile_set_registers(mc, map, tile, all_tiles, ntiles);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_SUCCESS;
}

/**
 * Performance miscellaneous initialization of tiles.
 * @param[in] mc      A manycore instance.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tiles   The list of tiles to initialize.
 * @param[in] ntiles  The number of tiles to initialize.
 * @return HB_MC_SUCCESS if an error occured. Otherwise an error code is returned.
 */
static int hb_mc_loader_tiles_initialize(hb_mc_manycore_t *mc,
					 const hb_mc_eva_map_t *map,
					 const hb_mc_coordinate_t *tiles,
					 uint32_t ntiles)
{
	int rc;

	if (ntiles == 0)
		return HB_MC_INVALID;

	for (uint32_t i = 0; i < ntiles; i++) {
		rc = hb_mc_loader_tile_initialize(mc, map, tiles[i], tiles, ntiles);
		if (rc != HB_MC_SUCCESS)
			return rc;
	}

	return HB_MC_SUCCESS;
}

/**
 * Loads an ELF file into a list of tiles and DRAM
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tiles  A list of manycore to load with #bin, with the origin at 0
 * @param[in]  ntiles The number of tiles in #tiles
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_loader_load(const void *bin, size_t sz, hb_mc_manycore_t *mc,
		      const hb_mc_eva_map_t *map,
		      const hb_mc_coordinate_t *tiles, uint32_t ntiles)
{
	int rc;

	if (ntiles < 1)
		return HB_MC_INVALID;

        // Validate ELF File
	rc = hb_mc_loader_elf_validate(bin, sz);
	if (rc != HB_MC_SUCCESS)
		return rc;

        // Load segments
	rc = hb_mc_loader_load_segments(bin, sz, mc, map, tiles, ntiles);
	if (rc != HB_MC_SUCCESS)
		return rc;

	// Set CSRs
	rc = hb_mc_loader_tiles_initialize(mc, map, tiles, ntiles);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_SUCCESS;
}
