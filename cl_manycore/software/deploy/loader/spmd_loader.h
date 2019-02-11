#ifndef SPMD_LOADER_H
#define SPMD_LOADER_H
/*! \file spmd_loader.h
 * Helper functions to program the Manycore.
 */


#include "elf.h"
#include "bsg_manycore_pkt.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "../fifo.h"
#include "../bits.h"
/*------------------------------------------------------------------------------*/
// helper functions 
/*------------------------------------------------------------------------------*/
 
/*! 
 * Prints a manycore packet in hex. Bytes are separated by spaces.
 * @param p array of bytes.
 * */
void print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) { 
		printf("%x ", (p[15-i] & 0xFF));
	}
	printf("\n");
}

/*!
 * Helper function of self field.
 * @param data pointer to byte to modify.
 * @param start bit offset within byte of where field begins.
 * @param val the value to set the selected bits to.
 * */
void set_bits (uint8_t *data, uint8_t start, uint8_t val) {
	/* bits [start, start + size) to be 1 */
	*data |= val << start;
}

/*!
 * Sets a selected number of bits of a Manycore packet to a desired value.
 * @param packet an array of bytes that form the Manycore packet.
 * @param bit_start the bit offset within the packet where the field starts.
 * @param bit_end the bit offset within the packet where the field ends - inclusive.
 * @param val the value to set the selected bits to.
 * */
void set_field (uint8_t *packet, uint8_t bit_start, uint8_t bit_end, uint32_t val) {
	uint8_t byte_start = bit_start / 8;
	uint8_t byte_end = bit_end / 8;
	uint8_t byte_start_ofs = bit_start % 8;
	uint8_t byte_end_ofs = bit_end % 8;

	uint32_t done_bits = 0; 
	for (int byte = byte_start; byte <= byte_end; byte++) {
		if (byte == byte_start && byte == byte_end) {
			set_bits(&packet[byte], byte_start_ofs, val);
			done_bits += (byte_end_ofs - byte_start_ofs) + 1;
		}

		else if (byte == byte_start && byte != byte_end) {
			uint8_t val_ = get_bits(val, 0, 8 - byte_start_ofs);
			set_bits(&packet[byte], byte_start_ofs, val_);
			done_bits += (8 - byte_start_ofs);
		}

		else if (byte == byte_end) {
			uint8_t val_ = get_bits(val, done_bits, byte_end_ofs + 1);
			set_bits(&packet[byte], 0, val_);
			done_bits += byte_end_ofs + 1;
		}
			
		else { /* byte is not byte_start nor is it byte_end */
			uint8_t val_ = get_bits(val, done_bits, 8);
			set_bits(&packet[byte], 0, val_);
			done_bits += 8;
		}

	}
}

/*!
 * Forms a Manycore packet. Manycore fields are assumed to be less than 32 bits. Supports arbitrary Manycore dimensions.
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * @return array of bytes that form the Manycore packet.
 * assumes all fields are <= 32
 * */
uint8_t *get_pkt(uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode) {
	
	uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t));

	uint32_t bits_start = 0;

	set_field(packet, bits_start, bits_start + X_BIT-1, x); 
	bits_start += X_BIT;

	set_field(packet, bits_start, bits_start + Y_BIT - 1, y);
	bits_start += Y_BIT;

	set_field(packet, bits_start, bits_start + X_BIT - 1, MY_X);
	bits_start += X_BIT;

	set_field(packet, bits_start, bits_start + Y_BIT - 1, MY_Y);
	bits_start += Y_BIT;

	set_field(packet, bits_start, bits_start + DATA_BIT - 1, data);
	bits_start += DATA_BIT;
	
	set_field(packet, bits_start, bits_start + OP_EX_BIT - 1, 0xF);
	bits_start += OP_EX_BIT;

	set_field(packet, bits_start, bits_start + OP_BIT - 1, opcode);
	bits_start += OP_BIT;
	
	set_field(packet, bits_start, bits_start + ADDR_BIT - 1, addr);
	bits_start += ADDR_BIT;

	return packet;
}

/*------------------------------------------------------------------------------*/
// ICACHE Initialization 
/*------------------------------------------------------------------------------*/
/*!
 * Returns an array of Manycore packets that should be used to initialize the tiles' instruction caches. At the moment, only the top-left tile at (X, Y) = (0, 0) is initialized.
 * @return array of Manycore packets.
 * */
uint8_t **init_icache () {
	uint8_t **packets = (uint8_t **) calloc(NUM_ICACHE, sizeof(uint8_t *)); 
	for (int icache_addr = 0; icache_addr < NUM_ICACHE; icache_addr++) {
		int idx = icache_addr;
		packets[idx] = get_pkt((icache_addr | (1 << 22) ), 0, 0, 0, OP_REMOTE_STORE);
	}
	return packets;
}

/*------------------------------------------------------------------------------*/
// VCACHE Initialization 
/*------------------------------------------------------------------------------*/
/*!
 * Returns an array of Manycore packets that should be used to initialize the victim caches.
 * @return array of Manycore packets.
 * */
uint8_t **init_vcache () {
	uint32_t num_tags = NUM_VCACHE_ENTRY * VCACHE_WAYS;
	uint8_t **packets = (uint8_t **) calloc(NUM_VCACHE * num_tags, sizeof(uint8_t *)); 
	for (int cache = 0; cache < NUM_VCACHE; cache++) {
		for (int tag = 0; tag < num_tags; tag++) {
			uint32_t addr = (1 << (ADDR_BIT-1)) | (tag << 4);  
			packets[(cache * num_tags) + tag] = get_pkt(addr, 0, cache, NUM_Y+1, OP_REMOTE_STORE);
		}

	}
	return packets;
}

/*------------------------------------------------------------------------------*/
// Unfreeze tiles 
/*------------------------------------------------------------------------------*/
/*!
 * Returns an array of Manycore packets that should be used to unfreeze the needed tiles. Currently, on the the tile at (X, Y) = (0, 0) is unfrozen.
 * @return array of Manycore packets.
 * */

uint8_t **unfreeze_tiles (uint8_t x, uint8_t y) {
	uint8_t **packets = (uint8_t **) calloc(1, sizeof(uint8_t *)); 
	packets[0] = get_pkt((1 << 13), 0, x, y, OP_REMOTE_STORE);
	return packets;
}

/*------------------------------------------------------------------------------*/
// Get packets for loadable segments of the binary
// This code is based on https://github.com/riscv/riscv-fesvr/blob/master/fesvr/elfloader.cc
/*------------------------------------------------------------------------------*/
uint8_t **icache_pkts; /*! arrays of Manycore packets that contain the text segment of the binary to be sent to icache. */
uint8_t **text_pkts; /*! array of Manycore packets that contain the text segment of the binary to be sent to DRAM. */
uint8_t **data_pkts; /*! array of Manycore packets that contain the data segment of the binary. */

uint32_t num_text_pkts;
uint32_t num_data_pkts;

/*! 
 * Creates arrays of Manycore packets that contain the text and data segments of the binary. These arrays are saved in the global variables text_pkts and data_pkts.
 * @param filename the path to the binary.
 * */
void parse_elf (char *filename, uint8_t x, uint8_t y, bool init_dram) { 	
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
	
	uint32_t num_load = 0;
	
	num_text_pkts = 1 * (ph[TEXT].p_memsz / 4);
	if (init_dram)
		text_pkts = (uint8_t **) calloc(num_text_pkts, sizeof(uint8_t *));
	icache_pkts = (uint8_t **) calloc(num_text_pkts, sizeof(uint8_t *));

	num_data_pkts = (ph[DATA].p_memsz / 4); 
	data_pkts = (uint8_t **) calloc(num_data_pkts, sizeof(uint8_t *));
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
					int32_t addr = (ofs) >> 2; // ph[i].p_paddr
					uint32_t data = *((uint32_t *) (instructions + ofs));
					if (init_dram) {
						text_pkts[ofs/4] = get_pkt(addr, data, 0, NUM_Y+1, OP_REMOTE_STORE);
					}
					icache_pkts[ofs/4] = get_pkt(addr | (1 << 22), data, x, y, OP_REMOTE_STORE); /*  send packet to tile (0, 0) */
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
					data_pkts[ofs/4] = get_pkt(addr, data, x, y, OP_REMOTE_STORE);
				}
			}
			
		}
			
  	}
	munmap(buf, size);
}

/*!
 *  * writes the binary's instructions into (x,y)'s icache.
 *   * */
void load_icache() {
	int num_icache_packets = num_text_pkts;	
	bool pass = true;
	
	for (int i = 0; i < num_icache_packets; i++) {
		if (!deploy_write_fifo(0, (int *) icache_pkts[i])) {
			pass = false;
			break;
		}
	}
	if (pass) 
		printf("load_icache(): icache init finished.\n");
	else
		printf("load_icache(): icache init failed.\n");
}

/*!
 *  * writes the binary's data into (x,y)'s dmem.
 *   * */
void load_dmem() {
	bool pass = true;
	
	for (int i = 0; i < num_data_pkts; i++) {
		if (!deploy_write_fifo(0, (int *) data_pkts[i])) {
			pass = false;
			break;
		}
	}
	if (pass) 
		printf("loading the dmem finished.\n");
	else
		printf("loading the dmem failed.\n");
}

/*!
 *  * writes the binary's instructions into the DRAM at offset 0.
 *   * */
void load_dram() {
	printf("Loading DRAM with the binary's instructions ...\n");
	bool pass = true;

	for (int i = 0; i < num_text_pkts; i++) {
		if (!deploy_write_fifo(0, (int *) text_pkts[i])) {
			pass = false;
			break;
		}
		if (i % 1 == 0)
			printf("DRAM load in progress.\n");
	}
	if (pass) 
		printf("loading instructions into DRAM finished.\n");
	else
		printf("loading instructions into DRAM failed.\n");
}

/*!
 *  * unfreezes (x,y).
 *   * */
void unfreeze (uint8_t x, uint8_t y) {
	printf("Unfreezing tile (%d, %d).\n", x, y);
	uint8_t **unfreeze_pkts = unfreeze_tiles(x, y); 
	bool pass_unfreeze = true;
	if (!deploy_write_fifo(0, (int *) unfreeze_pkts[0])) {
		pass_unfreeze = false;
	}
	if (pass_unfreeze)
		printf("unfreeze finished.\n");
	else
		printf("unfreeze failed.\n");	
}

#endif 
