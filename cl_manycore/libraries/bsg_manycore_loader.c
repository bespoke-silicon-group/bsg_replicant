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

/*!
 *	* writes the binary's instructions into (x,y)'s icache.
 *	 * */
static int hb_mc_load_packets(uint8_t fd, uint8_t **pkts, uint32_t num_pkts) {

	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("load_packets(): warning - device was never initialized.\n");
		return HB_MC_FAIL;
	}
	
	int status = HB_MC_SUCCESS;
	for (int i = 0; i < num_pkts; i++) {
	packet_t packet;
	memcpy(&packet.request, (request_packet_t *) pkts[i], sizeof(packet_t));
	if (hb_mc_write_fifo(fd, 0, &packet) != HB_MC_SUCCESS) {
			status = HB_MC_FAIL;
			break;
		}
	}
	if (status != HB_MC_SUCCESS)
		printf("load_packets(): load failed.\n");
	return status;
}

/*!
 * Sets a selected number of bytes of a Manycore packet to a desired value.
 * @param packet an array of bytes that form the Manycore packet.
 * @param byte_start the byte offset within the packet where the field starts.
 * @param size the size in bytes of the field.
 * @param val the value to set the selected bytes to.
 * */
static void hb_mc_set_field (uint8_t *packet, uint8_t byte_start, uint8_t size, uint32_t val) {
	if (size == WORD) {
		uint32_t *field = (uint32_t *) (packet + byte_start);
		*field = val;
	}
	else if (size == SHORT) {
		uint16_t *field = (uint16_t *) (packet + byte_start);
		*field = val;
	}
	else {
		uint8_t *field = (uint8_t *) (packet + byte_start);
		*field = val;
	}
}

/*!
 * Forms a Manycore packet.
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * @return array of bytes that form the Manycore packet.
 * assumes all fields are <= 32
 * */
uint8_t *hb_mc_get_pkt(uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode) {
	
	uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t));

	uint32_t byte_start = 0;

	hb_mc_set_field(packet, byte_start, X_BYTE, x); 
	byte_start += X_BYTE;

	hb_mc_set_field(packet, byte_start, Y_BYTE, y);
	byte_start += Y_BYTE;

	hb_mc_set_field(packet, byte_start, X_BYTE, MY_X);
	byte_start += X_BYTE;

	hb_mc_set_field(packet, byte_start, Y_BYTE, MY_Y);
	byte_start += Y_BYTE;

	hb_mc_set_field(packet, byte_start, DATA_BYTE, data);
	byte_start += DATA_BYTE;
	
	hb_mc_set_field(packet, byte_start, OP_EX_BYTE, 0xF);
	byte_start += OP_EX_BYTE;

	hb_mc_set_field(packet, byte_start, OP_BYTE, opcode);
	byte_start += OP_BYTE;
	
	hb_mc_set_field(packet, byte_start, ADDR_BYTE, addr);
	byte_start += ADDR_BYTE;

	return packet;
}

static int hb_mc_get_elf_segment_size (char *filename, uint32_t *result_p, int segment) {
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
static void hb_mc_parse_elf (char *filename, uint8_t x, uint8_t y, uint32_t *num_instr, uint32_t *data_size, uint8_t ***icache_pkts, uint8_t ***dram_pkts, uint8_t ***dmem_pkts, bool init_dram) {	
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
	
	*num_instr = 1 * (ph[TEXT].p_memsz / 4);
	if (init_dram)
		*dram_pkts = (uint8_t **) calloc(*num_instr, sizeof(uint8_t *));
	*icache_pkts = (uint8_t **) calloc(*num_instr, sizeof(uint8_t *));

	*data_size = (ph[DATA].p_memsz / 4); 
	*dmem_pkts = (uint8_t **) calloc(*data_size, sizeof(uint8_t *));
	
	for (unsigned i = 0; i < eh->e_phnum; i++) { 
		if(ph[i].p_type == PT_LOAD && ph[i].p_memsz) { 
			if (i == TEXT) {
				uint8_t *instructions = (uint8_t *) calloc(ph[i].p_memsz, sizeof(uint8_t));
				if (ph[i].p_filesz) { 
						assert(size >= ph[i].p_offset + ph[i].p_filesz);  
					for (int byte = 0; byte < ph[i].p_filesz; byte++)
						instructions[byte] = buf[ph[i].p_offset + byte];
				}			
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
					int32_t addr = (ofs) >> 2; 
					uint32_t data = *((uint32_t *) (instructions + ofs));
					if (init_dram) {
						(*dram_pkts)[ofs/4] = hb_mc_get_pkt(addr, data, 0, hb_mc_get_num_y() + 1, OP_REMOTE_STORE);
					}
					(*icache_pkts)[ofs/4] = hb_mc_get_pkt(addr | (1 << 22), data, x, y, OP_REMOTE_STORE); /*  send packet to tile (0, 0) */
				}
			}
			
			else if (i == DATA) { /* load to tile (0, 0) */
				uint8_t *data_dmem = (uint8_t *) calloc(ph[i].p_memsz, sizeof(uint8_t));
				if (ph[i].p_filesz) { 
						assert(size >= ph[i].p_offset + ph[i].p_filesz);  
					for (int byte = 0; byte < ph[i].p_filesz; byte++)
						data_dmem[byte] = buf[ph[i].p_offset + byte];
				}		
				for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
					uint32_t addr = (4096 + ofs) >> 2;
					uint32_t data = *((uint32_t *) (data_dmem + ofs));
					(*dmem_pkts)[ofs/4] = hb_mc_get_pkt(addr, data, x, y, OP_REMOTE_STORE);
				}
			}
		}
	}
	munmap(buf, size);
}

void hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size) {
	printf("size of packet_t is 0x%x\n", sizeof(packet_t));
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("hb_mc_load_binary(): warning - device was not initialized.\n");
		return;
	}
	
	if (!size)
		return; 
	
	for (int i = 0; i < size; i++) {
		uint8_t **icache_pkts, **dram_pkts, **dmem_pkts;
		uint32_t num_instr, data_size;
		bool init_dram = (i == 0) ? true : false;
		hb_mc_parse_elf(filename, x[i], y[i], &num_instr, &data_size, &icache_pkts, &dram_pkts, &dmem_pkts, init_dram);
		printf("Loading icache of tile (%d, %d)\n", x[i], y[i]);
		hb_mc_load_packets(fd, icache_pkts, num_instr);
		if (init_dram) {
			printf("Loading dram.\n");
			hb_mc_load_packets(fd, dram_pkts, num_instr);
		}
		printf("Loading dmem of tile (%d, %d)\n", x[i], y[i]);
		hb_mc_load_packets(fd, dmem_pkts, data_size);
	}
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
		
	packet_t freeze; 
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
		
	packet_t unfreeze; 
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
	
	packet_t packet_origin_x, packet_origin_y;		
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
	packet_t tag;	
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

