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
#include <cstring>
#include <elf.h>
#include <endian.h>


typedef enum __hb_mc_loader_elf_field_t{
	HB_MC_LOADER_ELF_DATA_ID = 0,
	HB_MC_LOADER_ELF_TEXT_ID = 1,
	HB_MC_LOADER_ELF_DRAM_ID = 2,
} hb_mc_loader_elf_field_t;

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

static size_t hb_mc_loader_get_tile_segment_capacity(const void *bin, size_t sz,
						     hb_mc_manycore_t *mc,
						     hb_mc_coordinate_t tile,
						     hb_mc_loader_elf_field_t segment)
{
	return 0;
}

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
	size_t cap;
	
	/* get the segment info */
	rc = hb_mc_loader_elf_get_segment(bin, sz, segment, &phdr, &segptr);
	if (rc != HB_MC_SUCCESS)
		return rc;

	/* get hardware capacity of the segment */
	//cap = hb_mc_loader_get_tile_segment_capacity(bin, sz, mc, tile, segment);

	return HB_MC_FAIL;
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
	int rc;
	// Get Data segment
	// EVA memcopy
	return HB_MC_SUCCESS;
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
				       hb_mc_coordinate_t origin){
	int rc;
	// Get Text segment
	// EVA memcopy
	return HB_MC_SUCCESS;
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
				   hb_mc_coordinate_t origin){
	int rc;
	// rc = hb_mc_loader_load_dram_text();
	// rc = hb_mc_loader_load_dram_data();
	// load data
	// load text
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
	return HB_MC_SUCCESS;
}
