// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#define EM_RISCV        243     /* RISC-V */

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

        /* if the load EVA maps to DRAM */
        if (RV32_Addr_to_host(phdr->p_paddr) & (1<<31)) {
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
        snprintf(buffer, n, "segment @ 0x%08" PRIx32 " (size = 0x%08" PRIx32 ")",
                 RV32_Addr_to_host(phdr->p_vaddr),
                 RV32_Word_to_host(phdr->p_memsz));
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
                bsg_pr_err("%s: failed to write %s for tile (%d, %d)"
                           ": %s\n",
                           __func__,
                           segname,
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

        bsg_pr_dbg("%s: writing program data: %s\n", __func__, segname);

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
        hb_mc_eva_t eva = RV32_Addr_to_host(phdr->p_paddr); /* get the load eva */
        size_t file_sz = RV32_Word_to_host(phdr->p_filesz); /* get the size of segdata */

        rc = hb_mc_loader_eva_write(phdr, segdata, file_sz, eva, mc, map, tile);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: write: failed to load segment %s: %s\n",
                           __func__,
                           segname,
                           hb_mc_strerror(rc));
                return rc;
        }

        /* load zeroed data */
        size_t zeros_sz  = seg_sz - file_sz;  // zeros are the remainder of the segment
        eva += file_sz; // increment eva by number of initialized bytes written

        rc = hb_mc_loader_eva_memset(phdr, 0, zeros_sz, eva, mc, map, tile);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: memset: failed to load segment %s: %s\n",
                           __func__,
                           segname,
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
        if (((hb_mc_npa_get_epa(&icache_npa)) + sz - 1) & 0x00FFF000) {
                bsg_pr_dbg("%s: Oops: ICACHE EPA 0x%08" PRIx32 " sets tag bits\n",
                           __func__, hb_mc_npa_get_epa(&icache_npa));
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
{       int rc;

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

        /* first check if elf is null */
        if (elf == NULL)
                return HB_MC_INVALID;

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
        hb_mc_eva_t eva = RV32_Addr_to_host(phdr->p_paddr);
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
                if (rc != HB_MC_SUCCESS) {
                        bsg_pr_dbg("%s: failed to get segment %d\n", __func__, segidx);
                        return rc;
                }

                /* check if program header should be loaded never, once, or for each tile */
                if (hb_mc_loader_segment_is_load_never(mc, phdr, map, tiles, ntiles)) {
                        // this segment should not be loaded
                        continue;
                } else if (hb_mc_loader_segment_is_load_once(mc, phdr, map, tiles, ntiles)) {
                        // this segment should be loaded only once (e.g. DRAM = .text + .dram)
                        rc = hb_mc_loader_load_tile_segment(mc, map, phdr, segdata, tiles[0]);
                        if (rc != HB_MC_SUCCESS) {
                                return rc;
                        }
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

        return HB_MC_SUCCESS;
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

        rc = hb_mc_tile_freeze(mc, &tile);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to freeze (%" PRId32 ",%" PRId32 "): %s\n",
                           __func__,
                           hb_mc_coordinate_get_x(tile),
                           hb_mc_coordinate_get_y(tile),
                           hb_mc_strerror(rc));
                return rc;
        }

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

        /* set/clear DRAM enabled */
        if (hb_mc_manycore_dram_is_enabled(mc)) {
                rc = hb_mc_tile_set_dram_enabled(mc, &tile);
        } else {
                rc = hb_mc_tile_clear_dram_enabled(mc, &tile);
        }

        if (rc != HB_MC_SUCCESS) {
                char tile_str[32];
                bsg_pr_dbg("%s: failed to %s DRAM-enabled for %s: %s\n",
                           __func__,
                           hb_mc_manycore_dram_is_enabled(mc) ? "set" : "clear",
                           hb_mc_coordinate_to_string(tile, tile_str, sizeof(tile_str)),
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
 * Validate all victim cache tags.
 * @param[in] mc      A manycore instance.
 * @param[in] map     An EVA<->NPA map.
 * @param[in] tiles   The list of tiles to initialize.
 * @param[in] ntiles  The number of tiles to initialize.
 * @return HB_MC_SUCCESS if an error occured. Otherwise an error code is returned.
 */
static int hb_mc_loader_columns_validate_victim_cache(hb_mc_manycore_t *mc,
                                                      const hb_mc_eva_map_t *map)
{
	return hb_mc_manycore_validate_vcache(mc);
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

        /* validate all vcache tags if we're in no-DRAM mode */
        if (!hb_mc_manycore_dram_is_enabled(mc)) {
                rc = hb_mc_loader_columns_validate_victim_cache(mc, map);
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
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to validate binary\n", __func__);
                return rc;
        }

        // Set CSRs
        rc = hb_mc_loader_tiles_initialize(mc, map, tiles, ntiles);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to initialize tiles\n", __func__);
                return rc;
        }

        // Load segments
        rc = hb_mc_loader_load_segments(bin, sz, mc, map, tiles, ntiles);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to load segments\n", __func__);
                return rc;
        }

        return HB_MC_SUCCESS;
}

static int hb_mc_loader_get_section(const void *bin, size_t sz, unsigned idx,
                                    const Elf32_Shdr **shdr, const unsigned char **section_data)
{
        const unsigned char *program_data = (const unsigned char*)bin, *data;
        const Elf32_Ehdr *ehdr = (const Elf32_Ehdr*)bin;
        const Elf32_Shdr *section_table, *section;

        /* find the section table */
        size_t shoff = RV32_Off_to_host(ehdr->e_shoff);

        /* make sure that the section table is completely in bounds */
        if (shoff + RV32_Half_to_host(ehdr->e_shnum) * sizeof(Elf32_Shdr) > sz)
                return HB_MC_INVALID;

        section_table = (const Elf32_Shdr *)&program_data[shoff];
        section = &section_table[idx];

        /* get the section offset and size and check that it's valid */
        size_t section_off = RV32_Off_to_host(section->sh_offset);
        size_t section_sz = RV32_Word_to_host(section->sh_size);
        if (section_off + section_sz > sz) {
                bsg_pr_dbg("%s: Section %u: section_offset + section_size = %zu: but binary size = %zu\n",
                           __func__, idx, section_off+section_sz, sz);
                return HB_MC_INVALID;
        }

        /* return the header and data */
        *shdr = section;
        *section_data = &program_data[section_off];

        return HB_MC_SUCCESS;
}

static bool hb_mc_loader_section_is_symbol_table(const Elf32_Shdr *shdr)
{
        return RV32_Word_to_host(shdr->sh_type) == SHT_SYMTAB;
}

static int hb_mc_loader_symbol_search_symbol_table(const void *bin, size_t sz, const char *symbol,
                                                   const Elf32_Shdr *symtab_shdr, const unsigned char *symtab_data,
                                                   hb_mc_eva_t *eva)
{
        int rc;
        unsigned strtab_idx = RV32_Word_to_host(symtab_shdr->sh_link);
        const Elf32_Shdr *strtab_shdr;
        const unsigned char *strtab_data;
        const Elf32_Sym *symbol_table = (const Elf32_Sym*)symtab_data, *sym;
        const char *sym_name;

        /* get the string table for this section */
        rc = hb_mc_loader_get_section(bin, sz, strtab_idx,
                                      &strtab_shdr, &strtab_data);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to get section %u: %s\n",
                           __func__, strtab_idx, hb_mc_strerror(rc));
                return rc;
        }

        /* total number of symbols in symtab */
        Elf32_Word sym_n = RV32_Word_to_host(symtab_shdr->sh_size)/RV32_Word_to_host(symtab_shdr->sh_entsize);

        for (Elf32_Word sym_i = 0; sym_i < sym_n; sym_i++) {
                sym = &symbol_table[sym_i];

                Elf32_Word sym_name_off = RV32_Word_to_host(sym->st_name);

                /* skip symbols with no name */
                if (sym_name_off == 0)
                        continue;

                /* symbol's name is in bounds? */
                if (sym_name_off > RV32_Word_to_host(strtab_shdr->sh_size))
                        return HB_MC_INVALID;

                /* is this the symbol we're looking for? */
                sym_name = (const char *)&strtab_data[sym_name_off];
                if (strcmp(sym_name, symbol) == 0) {
                        *eva = RV32_Addr_to_host(sym->st_value);
                        return HB_MC_SUCCESS;
                }
        }

        /* for each symbol */
        return HB_MC_NOTFOUND;
}

static int hb_mc_loader_symbol_search_symbol_tables(const void *bin, size_t sz, const char *symbol,
                                                    hb_mc_eva_t *eva)
{
        Elf32_Ehdr *ehdr = (Elf32_Ehdr*) bin;
        const Elf32_Shdr *shdr;
        const unsigned char *section_data;
        size_t section_sz;
        int rc;

        for (unsigned idx = 0; idx < RV32_Half_to_host(ehdr->e_shnum); idx++) {
                rc = hb_mc_loader_get_section(bin, sz, idx, &shdr, &section_data);
                if (rc != HB_MC_SUCCESS) {
                        bsg_pr_dbg("%s: failed to get section %u: %s\n",
                                   __func__, idx, hb_mc_strerror(rc));
                        return rc;
                }

                if (!hb_mc_loader_section_is_symbol_table(shdr))
                        continue;

                rc = hb_mc_loader_symbol_search_symbol_table(bin, sz, symbol,
                                                             shdr, section_data,
                                                             eva);
                if (rc == HB_MC_NOTFOUND) {
                        continue;
                } else if (rc != HB_MC_SUCCESS) {
                        bsg_pr_dbg("%s: failed search for symbol '%s' in section %u: %s\n",
                                   __func__, symbol, idx, hb_mc_strerror(rc));
                        return rc;
                } else {
                        return rc;
                }
        }

        return HB_MC_NOTFOUND;
}

/**
 * Get an EVA for a symbol from a program data.
 * @param[in]  bin     A memory buffer containing a valid manycore binary.
 * @param[in]  sz      Size of #bin in bytes.
 * @param[in]  symbol  A program symbol.
 * @param[out] eva     An EVA that addresses #symbol.
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_loader_symbol_to_eva(const void *bin, size_t sz, const char *symbol,
                               hb_mc_eva_t *eva)
{
        int rc;

        if (!symbol || !eva)
                return HB_MC_INVALID;

        rc = hb_mc_loader_elf_validate(bin, sz);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to validate binary\n", __func__);
                return rc;
        }

        rc = hb_mc_loader_symbol_search_symbol_tables(bin, sz, symbol, eva);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_dbg("%s: failed to find symbol '%s': %s\n",
                           __func__,
                           symbol,
                           hb_mc_strerror(rc));
                return rc;
        }

        return HB_MC_SUCCESS;
}





/**
 * Takes in the path to a binary and loads the binary into a buffer and set the binary size.
 * @param[in]  file_name  Path and name of the binary file
 * @param[out] file_data  Pointer to the memory buffer to be loaded with a valid binary
 * @param[out] file_size  Size of binary in bytes.
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_loader_read_program_file(const char *file_name, unsigned char **file_data, size_t *file_size)
{
        struct stat st;
        FILE *f;
        int r;
        unsigned char *data;

        if ((r = stat(file_name, &st)) != 0) {
                bsg_pr_err("could not stat '%s': %m\n", file_name);
                return HB_MC_INVALID;
        }

        if (!(f = fopen(file_name, "rb"))) {
                bsg_pr_err("failed to open '%s': %m\n", file_name);
                return HB_MC_INVALID;
        }

        if (!(data = (unsigned char *) malloc(st.st_size))) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                return HB_MC_NOMEM;
        }

        if ((r = fread(data, st.st_size, 1, f)) != 1) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                free(data);
                return HB_MC_FAIL;
        }

        fclose(f);
        *file_data = data;
        *file_size = st.st_size;
        return HB_MC_SUCCESS;
}
