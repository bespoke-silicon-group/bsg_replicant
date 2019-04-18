#include <string.h>
#ifndef COSIM
	#include <bsg_manycore_driver.h> /* TODO: should be angle brackets */ 
	#include <bsg_manycore_loader.h>
	#include <bsg_manycore_errno.h>
#else
	#include <utils/sh_dpi_tasks.h>
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_loader.h"
	#include "bsg_manycore_errno.h"
#endif

uint32_t DMEM_BASE = 0x1000;
uint8_t MY_X = 3;
uint8_t MY_Y = 0; 
static uint32_t ICACHE_BASE_EPA = 1 < 22;

/*!
 *	* writes the binary's instructions into (x,y)'s icache.
 *	 * */
static int hb_mc_load_packets(uint8_t fd, hb_mc_packet_t *packets, uint32_t num_packets) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	
	for (int i = 0; i < num_packets; i++) {
		if (hb_mc_write_fifo(fd, 0, &packets[i]) != HB_MC_SUCCESS) {
			return HB_MC_FAIL;
		}
	}
	return HB_MC_SUCCESS;
}

static int hb_mc_get_elf_segment_size (char *filename, int segment, uint32_t *result_p) {
	int fd = open(filename, O_RDONLY);
	struct stat s;
	assert(fd != -1);
	if (fstat(fd, &s) < 0)
		return HB_MC_FAIL;	
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


static int hb_mc_parse_elf(char *filename, uint8_t x, uint8_t y, hb_mc_packet_t packets_icache[], hb_mc_packet_t packets_dram[], uint32_t text_size, hb_mc_packet_t packets_data[], uint32_t data_size, int init_dram) {
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
			if (i == TEXT) {
				uint32_t text_segment[ph[i].p_memsz / sizeof(uint32_t)];
				if (ph[i].p_filesz) { 
					if (size < ph[i].p_offset + ph[i].p_filesz)
						return HB_MC_FAIL;  
					memcpy(&text_segment[0], &buf[ph[i].p_offset], ph[i].p_filesz);
				}			
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
					uint32_t addr = (ofs) >> 2; 
					uint32_t data = text_segment[ofs/4];
					hb_mc_format_request_packet(&packets_icache[ofs/4].request, addr | ICACHE_BASE_EPA, data, x, y, OP_REMOTE_STORE);
					if (init_dram == HB_MC_SUCCESS) {
						hb_mc_format_request_packet(&packets_dram[ofs/4].request, addr, data, 0, hb_mc_get_num_y() + 1, OP_REMOTE_STORE);
					}
				}
			}
			else if (i == DATA) { 
				uint32_t data_segment[ph[i].p_memsz / sizeof(uint32_t)];
				if (ph[i].p_filesz) { 
					if (size < ph[i].p_offset + ph[i].p_filesz)
						return HB_MC_FAIL;
					memcpy(&data_segment[0], &buf[ph[i].p_offset], ph[i].p_filesz);	
				}		
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
					uint32_t addr = (DMEM_BASE + ofs) >> 2;
					uint32_t data = data_segment[ofs/4];
					hb_mc_format_request_packet(&packets_data[ofs/4].request, addr, data, x, y, OP_REMOTE_STORE);
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
	if (hb_mc_get_elf_segment_size(filename, TEXT, &text_size) != HB_MC_SUCCESS)	
		return HB_MC_FAIL;
	else if (hb_mc_get_elf_segment_size(filename, DATA, &data_size) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	
	hb_mc_packet_t packets_icache[text_size];
	hb_mc_packet_t packets_dram[text_size];
	hb_mc_packet_t packets_data[data_size];
	for (int i = 0; i < size; i++) {
		int init_dram = (i == 0) ? HB_MC_SUCCESS : HB_MC_FAIL; /* only load DRAM when loading the first tile */
		hb_mc_parse_elf(filename, x[i], y[i], packets_icache, packets_dram, text_size, packets_data, data_size, init_dram);
		hb_mc_load_packets(fd, packets_icache, text_size);
		if (init_dram == HB_MC_SUCCESS) {
			hb_mc_load_packets(fd, packets_dram, text_size);
		}
		hb_mc_load_packets(fd, packets_data, data_size);
	}
	return HB_MC_SUCCESS;
}

/*!
 * Freezes a Vanilla Core Endpoint.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_freeze (uint8_t fd, uint8_t x, uint8_t y) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
		
	hb_mc_packet_t freeze; 
	hb_mc_format_request_packet(&freeze.request, 1 << (EPA_BYTE_ADDR_WIDTH-3), 1, x, y, OP_REMOTE_STORE);
	if (hb_mc_write_fifo(fd, 0, &freeze) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	else
		return HB_MC_SUCCESS;

}

/*!
 * Unfreezes a Vanilla Core Endpoint.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
		
	hb_mc_packet_t unfreeze; 
	hb_mc_format_request_packet(&unfreeze.request, 1 << (EPA_BYTE_ADDR_WIDTH-3), 0, x, y, OP_REMOTE_STORE);
	if (hb_mc_write_fifo(fd, 0, &unfreeze) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	else
		return HB_MC_SUCCESS;
}

/*!
 * Sets a Vanilla Core Endpoint's tile group's origin.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @param[in] origin_x x coordinate of tile group's origin
 * @param[in] origin_y y coordinate of tile groups origin
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_set_tile_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t origin_x, uint8_t origin_y) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	
	hb_mc_packet_t packet_origin_x, packet_origin_y;		
	hb_mc_format_request_packet(&packet_origin_x.request, (1 << (EPA_BYTE_ADDR_WIDTH-3)) + CSR_TGO_X, origin_x, x, y, OP_REMOTE_STORE);
	hb_mc_format_request_packet(&packet_origin_y.request, (1 << (EPA_BYTE_ADDR_WIDTH-3)) + CSR_TGO_Y, origin_y, x, y, OP_REMOTE_STORE);
	if (hb_mc_write_fifo(fd, 0, &packet_origin_x) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	if (hb_mc_write_fifo(fd, 0, &packet_origin_y) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

/*!
 * Initializes a Vanilla Core Endpoint's instruction cache tag.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_init_cache_tag(uint8_t fd, uint8_t x, uint8_t y) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	hb_mc_packet_t tag;	
	hb_mc_format_request_packet(&tag.request, 1 << (EPA_TAG_ADDR_WIDTH-3), 0, x, y, OP_REMOTE_STORE);
		
	for (int i = 0; i < 4; i++) {
		#ifndef COSIM
		usleep(1);
		#else
		sv_pause(1);
		#endif
		if (hb_mc_write_fifo(fd, 0, &tag) != HB_MC_SUCCESS) {	
			return HB_MC_FAIL;
		}
	}
	return HB_MC_SUCCESS;
}

