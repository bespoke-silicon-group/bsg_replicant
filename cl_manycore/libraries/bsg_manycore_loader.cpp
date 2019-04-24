#include <string.h>
#ifndef COSIM
	#include <bsg_manycore_driver.h>
	#include <bsg_manycore_loader.h>
	#include <bsg_manycore_errno.h>
	#include <bsg_manycore_packet.h>
	#include <bsg_manycore_mmio.h>
	#include <bsg_manycore_epa.h>
#else
	#include <utils/sh_dpi_tasks.h>
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_loader.h"
	#include "bsg_manycore_errno.h"
	#include "bsg_manycore_packet.h"
	#include "bsg_manycore_mmio.h"
	#include "bsg_manycore_epa.h"
#endif

typedef enum __hb_mc_loader_elf_field_t{
	HB_MC_LOADER_ELF_DATA_ID = 0,
	HB_MC_LOADER_ELF_TEXT_ID = 1
} hb_mc_loader_elf_field_t;

/*!
 *	* writes the binary's instructions into (x,y)'s icache.
 *	 * */
static int hb_mc_load_packets(uint8_t fd, hb_mc_packet_t *packets, uint32_t num_packets) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	
	for (int i = 0; i < num_packets; i++) {
		if (hb_mc_write_fifo(fd, HB_MC_MMIO_FIFO_TO_HOST, 
					&packets[i]) != HB_MC_SUCCESS) {
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


static int hb_mc_parse_elf(char *filename, uint8_t x, uint8_t y, hb_mc_packet_t packets_icache[], hb_mc_packet_t packets_dram[], uint32_t text_size, hb_mc_packet_t packets_data[], uint32_t data_size, char init_dram) {
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
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
					uint32_t icache_word_addr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_ICACHE_BASE, ofs);
					uint32_t dram_word_addr = hb_mc_tile_epa_get_word_addr(
						hb_mc_tile_epa_get_byte_addr(HB_MC_VCACHE_EPA_BASE, HB_MC_VCACHE_EPA_DRAM_OFFSET), ofs);						
					uint32_t data = text_segment[ofs/4];
					hb_mc_format_request_packet(&packets_icache[ofs/4].request,
								icache_word_addr, 
								data, x, y, 
								HB_MC_PACKET_OP_REMOTE_STORE);
					if (init_dram) {
						hb_mc_format_request_packet(&packets_dram[ofs/4].request, dram_word_addr, data, 0, hb_mc_get_manycore_dimension_y() + 1, HB_MC_PACKET_OP_REMOTE_STORE);
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
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
					uint32_t dmem_word_addr = hb_mc_tile_epa_get_word_addr(HB_MC_TILE_EPA_DMEM_BASE, ofs);
					uint32_t data = data_segment[ofs/4];
					hb_mc_format_request_packet(&packets_data[ofs/4].request,
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

int hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
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
		hb_mc_parse_elf(filename, x[i], y[i], packets_icache, packets_dram, text_size, packets_data, data_size, init_dram);
		hb_mc_load_packets(fd, packets_icache, text_size);
		if (init_dram) {
			hb_mc_load_packets(fd, packets_dram, text_size);
		}
		hb_mc_load_packets(fd, packets_data, data_size);
	}
	return HB_MC_SUCCESS;
}
