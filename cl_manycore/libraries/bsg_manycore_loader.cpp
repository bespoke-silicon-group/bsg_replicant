#define DEBUG
#ifndef COSIM
#include <bsg_manycore_loader.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_eva.h>
#else
#include "bsg_manycore_loader.h"
#include "bsg_manycore_printing.h"
#include "bsg_manycore_npa.h"
#include "bsg_manycore_eva.h"
#endif
#include <cinttypes>
#include <cstring>
#include <elf.h>
#include <endian.h>

static size_t min_size_t(size_t x, size_t y)
{
	return x < y ? x : y;
}


typedef enum __hb_mc_loader_elf_field_t{
	HB_MC_LOADER_ELF_DATA_ID = 0,
	HB_MC_LOADER_ELF_TEXT_ID = 1,
	HB_MC_LOADER_ELF_DRAM_ID = 2,
} hb_mc_loader_elf_field_t;

const char * hb_mc_loader_elf_field_to_string(hb_mc_loader_elf_field_t segment)
{
	static const char *strtab [] = {
		[HB_MC_LOADER_ELF_DATA_ID] = "dmem data",
		[HB_MC_LOADER_ELF_TEXT_ID] = "program text",
		[HB_MC_LOADER_ELF_DRAM_ID] = "program data",
	};
	return strtab[segment];
}

typedef hb_mc_loader_elf_field_t hb_mc_segment_t; // This should replace above

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
 * Gets a pointer to a segment in the ELF binary
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  id     The segment id to read from the ELF file
 * @param[out] phdr   A pointer to the program header for this segment.
 * @param[out] seg    A pointer to segment location in #bin
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_elf_get_segment(const void *bin, size_t sz,
					hb_mc_segment_t id,
					Elf32_Phdr **phdr, void **seg)
{
	int rc;
	return HB_MC_FAIL;
}

/**
 * Get the max memory capacity of a program segment (these map to hardware).
 * @param[in] bin      Program data (unused)
 * @param[in] sz       The size of program data.
 * @param[in] mc       A manycore instance.
 * @param[in] tile     A manycore coordinate.
 * @param[in] segment  A segment ID.
 * @return the max size for #segment.
 */
static size_t hb_mc_loader_get_tile_segment_capacity(const void *bin, size_t sz,
						     hb_mc_manycore_t *mc,
						     hb_mc_coordinate_t tile,
						     hb_mc_loader_elf_field_t segment)
{
	switch (segment) {
	case HB_MC_LOADER_ELF_DATA_ID:
		return hb_mc_manycore_get_dmem_size(mc, &tile);
	case HB_MC_LOADER_ELF_TEXT_ID:
		return hb_mc_manycore_get_dram_size(mc);
	case HB_MC_LOADER_ELF_DRAM_ID:
		return hb_mc_manycore_get_dram_size(mc);
	}

	return 0;
}

/**
 * Writes zeros to an NPA.
 * @param[in]  mc     A manycore instance.
 * @param[in]  start  A start NPA.
 * @param[in]  sz     How many zero bytes to write.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
static int hb_mc_loader_write_zeros_to_page(hb_mc_manycore_t *mc,
					    const hb_mc_npa_t *start,
					    size_t sz)
{
	size_t words = sz >> 2;
	hb_mc_npa_t npa = *start;
	int rc;

	for (size_t i = 0; i < words; i++) {
		rc = hb_mc_manycore_write32(mc, &npa, 0);
		if (rc != HB_MC_SUCCESS)
			return rc;

                /* increment by 1 -- EPAs are word addressed */
		hb_mc_npa_set_epa(&npa, hb_mc_npa_get_epa(&npa)+1);
	}

	return HB_MC_SUCCESS;
}

/**
 * Writes program data to an EVA.
 * @param[in] data       Data to be written out.
 * @param[in] start_eva  The start EVA.
 * @param[in] mc         A manycore instance.
 * @param[in] id         And EVA space id.
 * @param[in] tile       A manycore coordinate.
 * @param[in] segment    A segment ID (for debugging).
 * @param[in] zeros      If true, zeros will be written instead of data.
 * @return HB_MC_SUCCESS if succesful. Otherwise and error code is returned.
 */
static int hb_mc_loader_write_to_eva(const unsigned char *data, size_t sz,
				     hb_mc_eva_t start_eva,
				     hb_mc_manycore_t *mc,
				     const hb_mc_eva_id_t *id,
				     hb_mc_coordinate_t tile,
				     hb_mc_loader_elf_field_t segment,
				     bool zeros = false)
{
	hb_mc_eva_t eva = start_eva;
	size_t off = 0, rem = sz;
	int rc;

	/* while there's data left to write */
	while (rem > 0) {
		hb_mc_npa_t npa;
		size_t page_sz;

		/* translate the EVA */
		rc = hb_mc_eva_to_npa(hb_mc_manycore_get_config(mc), id, &tile,
				      &eva, &npa, &page_sz);
		if (rc != HB_MC_SUCCESS) {
			bsg_pr_err("%s: %s: writing %s: "
				   "EVA 0x%08" PRIx32 " does not map to any NPA\n",
				   __func__,
				   hb_mc_eva_id_get_name(id),
				   hb_mc_loader_elf_field_to_string(segment),
				   eva);
			return rc;
		}

		/* write min(page_sz, file_sz) to npa and then increment eva by page_sz */
		size_t cpy_sz = min_size_t(page_sz, rem);

		if (zeros) { /* write zeros to this page */
			rc = hb_mc_loader_write_zeros_to_page(mc, &npa, cpy_sz);
		} else { /* write from data */
			rc = hb_mc_manycore_write_mem(mc, &npa, &data[off], cpy_sz);
		}

		/* report error? */
		if (rc != HB_MC_SUCCESS) {
			bsg_pr_err("%s: failed to write %s @ offset 0x%08zx: %s\n",
				   __func__, hb_mc_loader_elf_field_to_string(segment),
				   off, hb_mc_strerror(rc));
			return rc;
		}

		/* update off, rem, and eva */
		off += cpy_sz;
		rem -= cpy_sz;
		eva += cpy_sz;
	}

	return HB_MC_SUCCESS;
}

/**
 * Load a program segment.
 * @param[in] bin      A pointer to pogram data.
 * @param[in] sz       The size of the program data in #bin.
 * @param[in] id       A EVA space ID.
 * @param[in] tile     A manycore coordinate.
 * @param[in] segment  A segment ID.
 * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
 */
static int hb_mc_loader_load_tile_segment(const void *bin, size_t sz,
					  hb_mc_manycore_t *mc,
					  const hb_mc_eva_id_t *id,
					  hb_mc_coordinate_t tile,
					  hb_mc_loader_elf_field_t segment)
{
	Elf32_Phdr *phdr;
	int rc;
	void *segptr;
	unsigned char *segdata;
	size_t cap, seg_sz;

	/* get the segment info */
	rc = hb_mc_loader_elf_get_segment(bin, sz, segment, &phdr, &segptr);
	if (rc != HB_MC_SUCCESS)
		return rc;

	segdata = (unsigned char *)segptr;

	/* get hardware capacity of the segment */
	cap = hb_mc_loader_get_tile_segment_capacity(bin, sz, mc, tile, segment);
	seg_sz = RV32_Word_to_host(phdr->p_memsz);

        /* return error if the hardware lacks the capacity */
	if (cap < seg_sz) {
		bsg_pr_err("%s: segment '%s' (%zu bytes) exceeds "
			   "maximum (%zu bytes)\n",
			   __func__,
			   hb_mc_loader_elf_field_to_string(segment),
			   seg_sz,
			   cap);
		return HB_MC_FAIL;
	}

	/* load initialized data */
	hb_mc_eva_t eva = RV32_Addr_to_host(phdr->p_vaddr); /* get the eva */
	size_t file_sz = RV32_Word_to_host(phdr->p_filesz); /* get the size of segdata */

	rc = hb_mc_loader_write_to_eva(segdata, file_sz, eva, mc, id, tile, segment, false);
	if (rc != HB_MC_SUCCESS)
		return rc;

	/* load zeroed data */
	size_t zeros_sz  = seg_sz - file_sz;  // zeros are the remainder of the segment
	eva += file_sz; // increment eva by number of initialized bytes written

	rc = hb_mc_loader_write_to_eva(NULL, zeros_sz, eva, mc, id, tile, segment, true);
	if (rc != HB_MC_SUCCESS)
		return rc;

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
 * Loads an ELF file into the instruction cache of a tile
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  tile   The target manycore tile
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_tile_icache(const void *bin, size_t sz,
					 hb_mc_manycore_t *mc,
					 const hb_mc_eva_id_t *id, hb_mc_coordinate_t tile)
{
	int rc;

	// Get text segment
	// Get ICache Size (How?)
	// EVA memcopy
	return HB_MC_SUCCESS;
}

/**
 * Loads an ELF file into the data memory of a tile
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  tile   The target manycore tile
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_tile_dmem(const void *bin, size_t sz,
				       hb_mc_manycore_t *mc,
				       const hb_mc_eva_id_t *id, hb_mc_coordinate_t tile)
{
	return hb_mc_loader_load_tile_segment(bin, sz, mc, id, tile, HB_MC_LOADER_ELF_DATA_ID);
}

/**
 * Loads an ELF file into the data memory and instruction cache of a tile
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  tile   The target manycore tile
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_tile(const void *bin, size_t sz,
				  hb_mc_manycore_t *mc,
				  const hb_mc_eva_id_t *id,
				  hb_mc_coordinate_t tile)
{
	int rc;

	// Load DMEM
	rc = hb_mc_loader_load_tile_dmem(bin, sz, mc, id, tile);
	if (rc != HB_MC_SUCCESS)
		return rc;

	// Load ICache
	rc = hb_mc_loader_load_tile_icache(bin, sz, mc, id, tile);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_FAIL;
}

/**
 * Loads an ELF file into a list of tiles
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  tiles  A list of manycore to load with #bin, with the origin at 0
 * @param[in]  ntiles The number of tiles in #tiles
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_tiles(const void *bin, size_t sz,
				   hb_mc_manycore_t *mc,
				   const hb_mc_eva_id_t *id,
				   const hb_mc_coordinate_t *tiles,
				   uint32_t ntiles)
{
	uint32_t idx;
	int rc;

	// For each tile in tiles
	for (idx = 0; idx < ntiles; idx++) {
		// rc = hb_mc_load_tile()
		rc = hb_mc_loader_load_tile(bin, sz, mc, id, tiles[idx]);
		if (rc != HB_MC_SUCCESS)
			return rc;
	}

	return HB_MC_SUCCESS;
}

/**
 * Loads the text segment of an ELF file into DRAM
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  origin The origin tile of the tile group, for EVA->NPA conversion
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_dram_text(const void *bin, size_t sz,
				       hb_mc_manycore_t *mc, const hb_mc_eva_id_t *id,
				       hb_mc_coordinate_t origin)
{
	return hb_mc_loader_load_tile_segment(bin, sz, mc, id, origin, HB_MC_LOADER_ELF_TEXT_ID);
}

/**
 * Loads the data segment of an ELF file into DRAM
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  origin The origin tile of a group, for EVA->NPA conversion
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_dram_data(const void *bin, size_t sz,
				       hb_mc_manycore_t *mc, const hb_mc_eva_id_t *id,
				       hb_mc_coordinate_t origin)
{
	return hb_mc_loader_load_tile_segment(bin, sz, mc, id, origin, HB_MC_LOADER_ELF_DRAM_ID);
}

/**
 * Loads an ELF binary into DRAM
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  origin The origin tile of a group, for EVA->NPA conversion
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_load_drams(const void *bin, size_t sz,
				   hb_mc_manycore_t *mc, const hb_mc_eva_id_t *id,
				   hb_mc_coordinate_t origin)
{
	int rc;

	/* I'm pretty sure that text and data are the same segment... */
	/* This should be refactored to loop over DRAM segments */
	rc = hb_mc_loader_load_dram_text(bin, sz, mc, id, origin);
	if (rc != HB_MC_SUCCESS)
		return rc;

	rc = hb_mc_loader_load_dram_data(bin, sz, mc, id, origin);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_SUCCESS;
}

/**
 * Loads an ELF file into a list of tiles and DRAM
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  tiles  A list of manycore to load with #bin, with the origin at 0
 * @param[in]  ntiles The number of tiles in #tiles
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_loader_load(const void *bin, size_t sz, hb_mc_manycore_t *mc,
		      const hb_mc_eva_id_t *id, const hb_mc_coordinate_t *tiles, uint32_t ntiles)
{
	int rc;
	// Validate ELF File
	rc = hb_mc_loader_elf_validate(bin, sz);
	if (rc != HB_MC_SUCCESS)
		return rc;

	// Load tiles
	rc = hb_mc_loader_load_tiles(bin, sz, mc, id, tiles, ntiles);
	if (rc != HB_MC_SUCCESS)
		return rc;

	// Load DRAMs
	rc = hb_mc_loader_load_drams(bin, sz, mc, id, tiles[0]);
	if (rc != HB_MC_SUCCESS)
		return rc;

	return HB_MC_SUCCESS;
}
