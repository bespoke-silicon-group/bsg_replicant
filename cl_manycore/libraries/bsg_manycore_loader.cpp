#define DEBUG
#ifndef COSIM
#include <bsg_manycore_loader.h>
#include <bsg_manycore_printing.h>
#else
#include "bsg_manycore_loader.h"
#include "bsg_manycore_printing.h"
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

#define COMPILE_ASHS_CODE 0
#if COMPILE_ASHS_CODE
/*!
 *	* writes the binary's instructions into (x,y)'s icache.
 *	 * */
static int hb_mc_load_packets(uint8_t fd, hb_mc_packet_t *packets, uint32_t num_packets) {
	int rc;
	rc = hb_mc_fifo_check(fd);
	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	for (int i = 0; i < num_packets; i++) {
		rc = hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &packets[i]);
		if (rc != HB_MC_SUCCESS) {
			return HB_MC_FAIL;
		}
	}
	return HB_MC_SUCCESS;
}

static int hb_mc_get_elf_segment_size (char *filename, int segment, uint32_t *result_p) {
	int fd = open(filename, O_RDONLY);
	struct stat s;
	if (fd == -1) {
		fprintf(stderr, "hb_mc_get_elf_segment_size(): could not open the elf file %s\n", filename);
		return HB_MC_FAIL;
	}

	if (fstat(fd, &s) < 0) {
		close(fd);
		return HB_MC_FAIL;
	}
	size_t elf_size = s.st_size;
	uint8_t* buf = (uint8_t*) mmap(NULL, elf_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	if (buf == MAP_FAILED)
		return HB_MC_FAIL;
	else if (elf_size < sizeof(Elf64_Ehdr))
		return HB_MC_FAIL;

	Elf32_Ehdr* eh = (Elf32_Ehdr *) buf;
	Elf32_Phdr* ph = (Elf32_Phdr *) (buf + eh->e_phoff);
	if (elf_size < (eh->e_phoff + eh->e_phnum*sizeof(*ph)))
		return HB_MC_FAIL;

	*result_p = ph[segment].p_memsz / sizeof(uint32_t);
	return HB_MC_SUCCESS;
}

/*!
 * Creates arrays of Manycore packets that contain the text and data segments of the binary. These arrays are saved in the global variables text_pkts and data_pkts.
 * @param filename the path to the binary.
 * */


static int hb_mc_parse_elf(uint8_t device_fd, char *filename, uint8_t x, uint8_t y, hb_mc_packet_t packets_icache[], hb_mc_packet_t packets_dram[], uint32_t text_size, hb_mc_packet_t packets_data[], uint32_t data_size, char init_dram) {
	int fd = open(filename, O_RDONLY);
	struct stat s;
	assert(fd != -1);
	if (fstat(fd, &s) < 0)
		abort();
	size_t size = s.st_size;

	uint8_t* buf = (uint8_t*) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf != MAP_FAILED);
	close(fd);

	assert(size >= sizeof(Elf64_Ehdr));

	Elf32_Ehdr* eh = (Elf32_Ehdr *) buf;
	Elf32_Phdr* ph = (Elf32_Phdr *) (buf + eh->e_phoff);
	assert(size >= eh->e_phoff + eh->e_phnum*sizeof(*ph));

	for (unsigned i = 0; i < eh->e_phnum; i++) {
		if(ph[i].p_type == PT_LOAD && ph[i].p_memsz) {
			if (i == HB_MC_LOADER_ELF_TEXT_ID) {
				uint32_t text_segment[ph[i].p_memsz / sizeof(uint32_t)];
				if (ph[i].p_filesz) {
					if (size < ph[i].p_offset + ph[i].p_filesz)
						return HB_MC_FAIL;
					memcpy(&text_segment[0], &buf[ph[i].p_offset], ph[i].p_filesz);
				}
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += sizeof(uint32_t)) {
					// For the TEXT segment, we initialize
					// both DRAM and the icache. The DRAM is
					// abstracted by the VCACHE so we use
					// the address macros for VCACHE to
					// write into the DRAM
					uint32_t icache_word_addr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_ICACHE_BASE, ofs);
					uint32_t dram_word_addr = hb_mc_tile_epa_get_word_addr(
						hb_mc_tile_epa_get_byte_addr(HB_MC_VCACHE_EPA_BASE, HB_MC_VCACHE_EPA_DRAM_OFFSET), ofs);
					uint32_t data = text_segment[ofs/sizeof(uint32_t)];
					hb_mc_format_request_packet(device_fd, &packets_icache[ofs/sizeof(uint32_t)].request,
								    icache_word_addr,
								    data, x, y,
								    HB_MC_PACKET_OP_REMOTE_STORE);
					if (init_dram) {
						hb_mc_format_request_packet(device_fd, &packets_dram[ofs/sizeof(uint32_t)].request, dram_word_addr, data, 0, hb_mc_get_manycore_dimension_y() + 1, HB_MC_PACKET_OP_REMOTE_STORE);
					}
				}
			}
			else if (i == HB_MC_LOADER_ELF_DATA_ID) {
				uint32_t data_segment[ph[i].p_memsz / sizeof(uint32_t)];
				if (ph[i].p_filesz) {
					if (size < ph[i].p_offset + ph[i].p_filesz)
						return HB_MC_FAIL;
					memcpy(&data_segment[0], &buf[ph[i].p_offset], ph[i].p_filesz);
				}
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += sizeof(uint32_t)) {
					uint32_t dmem_word_addr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_DMEM_BASE, ofs);
					uint32_t data = data_segment[ofs/sizeof(uint32_t)];
					hb_mc_format_request_packet(device_fd, &packets_data[ofs/sizeof(uint32_t)].request,
								    dmem_word_addr, data,
								    x, y,
								    HB_MC_PACKET_OP_REMOTE_STORE);
				}
			}
		}
	}
	munmap(buf, size);
	return HB_MC_SUCCESS;
}

int hb_mc_load_binary(uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size) {
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	else if (!size)
		return HB_MC_FAIL; /* 0 tiles */

	uint32_t text_size, data_size;
	if (hb_mc_get_elf_segment_size(filename, HB_MC_LOADER_ELF_TEXT_ID, &text_size) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	else if (hb_mc_get_elf_segment_size(filename, HB_MC_LOADER_ELF_DATA_ID, &data_size) != HB_MC_SUCCESS)
		return HB_MC_FAIL;

	hb_mc_packet_t packets_icache[text_size];
	hb_mc_packet_t packets_dram[text_size];
	hb_mc_packet_t packets_data[data_size];
	for (int i = 0; i < size; i++) {
		/* only load DRAM when loading the first tile */
		char init_dram = (i == 0);
		hb_mc_parse_elf(fd, filename, x[i], y[i], packets_icache, packets_dram, text_size, packets_data, data_size, init_dram);
		hb_mc_load_packets(fd, packets_icache, text_size);
		if (init_dram) {
			hb_mc_load_packets(fd, packets_dram, text_size);
		}
		hb_mc_load_packets(fd, packets_data, data_size);
	}
	return HB_MC_SUCCESS;
}
#endif

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
 * @param[out] seg    A pointer to segment location in #bin
 * @param[out] segsz  The number of bytes in the segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_loader_elf_get_segment(const void *bin, size_t sz,
					hb_mc_segment_t id,
					Elf32_Phdr **phdr, void **seg, size_t *segsz)
{
	int rc;
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
					 const hb_mc_manycore_t *mc,
					 const eva_id_t *id, hb_mc_coordinate_t tile)
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
				       const hb_mc_manycore_t *mc,
				       const eva_id_t *id, hb_mc_coordinate_t tile)
{
	Elf32_Phdr *phdr; // the program header for dmem
	size_t dmem_cap; // the capacity of the tile's data memory
	size_t dmem_sz;  // the size of the data segment
	void *dmem; // pointer to the data segment
	int rc;

	// Get Data Segment
	rc = hb_mc_loader_elf_get_segment(bin, sz, HB_MC_LOADER_ELF_DATA_ID,
					  &phdr, &dmem, &dmem_sz);
	if (rc != HB_MC_SUCCESS)
		return rc;

	// Get DMem Size
	dmem_cap = hb_mc_manycore_get_dmem_size(mc, &tile);

	// EVA memcopy

	return HB_MC_SUCCESS;
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
				  const hb_mc_manycore_t *mc,
				  const eva_id_t *id,
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
				   const hb_mc_manycore_t *mc,
				   const eva_id_t *id,
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
				       const hb_mc_manycore_t *mc, const eva_id_t *id,
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
				       const hb_mc_manycore_t *mc, const eva_id_t *id,
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
				   const hb_mc_manycore_t *mc, const eva_id_t *id,
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
int hb_mc_loader_load(const void *bin, size_t sz, const hb_mc_manycore_t *mc,
		      const eva_id_t *id, const hb_mc_coordinate_t *tiles, uint32_t ntiles)
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
