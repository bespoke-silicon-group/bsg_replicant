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
 * @param[in] mc       A manycore instance.
 * @param[in] id       An EVA id.
 * @param[in] phdr     A program header for the segment.
 * @param[in] tile     A manycore coordinate.
 * @return the max size for #segment.
 */
static size_t hb_mc_loader_get_tile_segment_capacity(hb_mc_manycore_t *mc,
						     const hb_mc_eva_id_t *id,
						     const Elf32_Phdr *phdr,
						     hb_mc_coordinate_t tile)
{
	/*
	  The right thing to do here would to get the hardware component 
	  from the NPA, and then check the capacity of that.
	  
	  We don't currently have a function the maps NPAs to their 
	  underlying hardware type.

	  So for now we just check the top bit of the EVA.
	  If the top bit is 1, then address is DRAM (
	 */
	
	/* if the EVA maps to DRAM */
	if (RV32_Addr_to_host(phdr->p_vaddr) & (1<<31)) {
		return hb_mc_manycore_get_dram_size(mc);
	} else {
		return hb_mc_manycore_get_dmem_size(mc, &tile);
	}
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
 * @param[in] zeros      If true, zeros will be written instead of data.
 * @return HB_MC_SUCCESS if succesful. Otherwise and error code is returned.
 */
static int hb_mc_loader_write_to_eva(const Elf32_Phdr *phdr,
				     const unsigned char *data, size_t sz,
				     hb_mc_eva_t start_eva,
				     hb_mc_manycore_t *mc,
				     const hb_mc_eva_map_t *map,
				     hb_mc_coordinate_t tile,
				     bool zeros = false)
{
	hb_mc_eva_t eva = start_eva;
	size_t off = 0, rem = sz;
	int rc;
	char segname[64];

	hb_mc_loader_segment_to_string(phdr, segname, sizeof(segname));
	
	/* while there's data left to write */
	while (rem > 0) {
		hb_mc_npa_t npa;
		size_t page_sz;

		/* translate the EVA */
		rc = hb_mc_eva_to_npa(hb_mc_manycore_get_config(mc), map, &tile,
				      &eva, &npa, &page_sz);
		if (rc != HB_MC_SUCCESS) {
			bsg_pr_err("%s: %s: writing %s: "
				   "EVA 0x%08" PRIx32 " does not map to any NPA\n",
				   __func__,
				   hb_mc_eva_map_get_name(map),
				   segname,
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
				   __func__,
				   "anonymous segment",
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
 * @param[in] map      A EVA to NPA map.
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
	cap = hb_mc_loader_get_tile_segment_capacity(mc, id, phdr, tile);
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

	rc = hb_mc_loader_write_to_eva(phdr, segdata, file_sz, eva, mc, map, tile, false);
	if (rc != HB_MC_SUCCESS)
		return rc;

	/* load zeroed data */
	size_t zeros_sz  = seg_sz - file_sz;  // zeros are the remainder of the segment
	eva += file_sz; // increment eva by number of initialized bytes written
	
	rc = hb_mc_loader_write_to_eva(phdr, NULL, zeros_sz, eva, mc, map, tile, true);
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

static int hb_mc_loader_get_segment(const void *bin, size_t sz, int segidx,
				    const Elf32_Phdr **phdr, const unsigned char **segdata)
{
	return HB_MC_FAIL;
}
				    
static int hb_mc_loader_load_segments(const void *bin, size_t sz,
				      hb_mc_manycore_t *mc, const hb_mc_eva_map_t *map,
				      const hb_mc_coordinate_t *tiles, uint32_t ntiles)
{
	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)bin;
	int rc;

	/* for each program header */
	for (int segidx = 0; segidx < RV32_Half_to_host(ehdr->e_phnum); segidx++) {
		const Elf32_Phdr *phdr;
		const unsigned char *segdata;
		
		rc = hb_mc_loader_get_segment(bin, sz, segidx, &phdr, &segdata);
		if (rc != HB_MC_SUCCESS)
			return rc;

		/* decide if this segment is 'load once' or 'load for each tile' */
		if (hb_mc_loader_segment_is_load_once(mc, phdr, id, tiles, ntiles)) {
			rc = hb_mc_loader_load_tile_segment(mc, id, phdr, segdata, tiles[0]);
			if (rc != HB_MC_SUCCESS)
				return rc;
		} else { /* load on  each tile */			
		}
	}
	
	return HB_MC_FAIL;
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
		      const hb_mc_eva_map_t *map, const hb_mc_coordinate_t *tiles, uint32_t ntiles)
{
	int rc;
	// Validate ELF File
	rc = hb_mc_loader_elf_validate(bin, sz);
	if (rc != HB_MC_SUCCESS)
		return rc;

	rc = hb_mc_loader_load_segments(bin, sz, mc, map, tiles, ntiles);
	if (rc != HB_MC_SUCCESS)
		return rc;	

	return HB_MC_SUCCESS;
}
