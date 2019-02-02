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

/*------------------------------------------------------------------------------*/
// helper functions 
/*------------------------------------------------------------------------------*/
void print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) 
		printf("%x ", (int) (p[i] & 0xFF));
	printf("\n");
}

int32_t get_bits(uint32_t data, uint8_t start,  uint8_t size) {
	uint32_t mask = UINT_MAX;
	mask = mask << start;
	uint32_t bits = (data & mask) >> start;
	return bits;
}


/* assumes dimensions of Manycore are 4x4. 
 * */
uint8_t *get_pkt(uint32_t addr, uint32_t data, uint8_t x, uint8_t y) {
	uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t));
	
	/* byte 0 = MSB {LSB of src_y, src_x, y, x} LSB */
	packet[0] = x + (y << X_BIT) + (MY_X << (X_BIT + Y_BIT)) + (get_bits(MY_Y, 0, 1) << (X_BIT + Y_BIT + X_BIT));

	/* byte 1 = MSB {6 LSB of data, 2 MSB of src_y} LSB */
	packet[1] = get_bits(MY_Y, 1, 2) + (get_bits(data, 0, 6) << 2);

	/* bytes 2-4 = next 24b of data, */
	packet[2] = get_bits(data, 6, 8);
	packet[3] = get_bits(data, 14, 8);
	packet[4] = get_bits(data, 22, 8);
	
//	printf("data packets are: %x %x %x %x\n", packet[2], packet[3], packet[4], packet[5]);
	
	/* byte 5 = {op, op_ex, 2 MSB of data} */
	packet[5] = get_bits(data, 30, 2) + (0xF << 2) + (OP_REMOTE_STORE << (2 + OP_EX_BIT)); 

	/* bytes 6-8 are first 24b of address */
	packet[6] = get_bits(addr, 0, 8);
	packet[7] = get_bits(addr, 8, 8);
	packet[8] = get_bits(addr, 16, 8);

	/* byte 9 = MSB {0s, 2 MSB of address} LSB */
	packet[9] = get_bits(addr, 24, 2);

	/* byte 10-15 are 0s */
	return packet;
}

/* assumes dimensions of Manycore are 12x12. 
 * */

uint8_t *get_pkt_12x12(uint32_t addr, uint32_t data, uint8_t x, uint8_t y) {
	uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t));
	
	/* byte 0 = MSB {y, x} LSB */
	packet[0] = x + (y << X_BIT);

	/* byte 1 = MSB {src_y, src_x} LSB */
	packet[1] = MY_X + (MY_Y << X_BIT);

	/* bytes 2-5 = next 4B of data, */
	packet[2] = get_bits(data, 0, 8);
	packet[3] = get_bits(data, 8, 8);
	packet[4] = get_bits(data, 16, 8);
	packet[5] = get_bits(data, 24 ,8);	

	/* byte 6 = MSB {2 LSB of addr, op, op_ex} LSB */
	packet[6] = 0xF + (OP_REMOTE_STORE << OP_EX_BIT) + (get_bits(addr, 0, 2) << (OP_EX_BIT + OP_BIT)); 

	/* bytes 7-9 are next 24b of address */
	packet[7] = get_bits(addr, 2, 8);
	packet[8] = get_bits(addr, 10, 8);
	packet[9] = get_bits(addr, 18, 8);

	/* byte 10-15 are 0s */
	return packet;
}



/*------------------------------------------------------------------------------*/
// ICACHE Initialization 
/*------------------------------------------------------------------------------*/
/* returns the array of icache init packets. 
 * only initilizes the top-left tile @ (0, 0)
 * 
 * */
uint8_t **init_icache () {
	uint8_t **packets = (uint8_t **) calloc(NUM_ICACHE, sizeof(uint8_t *)); 
	for (int icache_addr = 0; icache_addr < NUM_ICACHE; icache_addr++) {
		int idx = icache_addr;
		packets[idx] = get_pkt_12x12(icache_addr, 0, 0, 0);
	}
	return packets;
}

/*------------------------------------------------------------------------------*/
// VCACHE Initialization 
/*------------------------------------------------------------------------------*/
/* returns the array of vcache init packets. */
uint8_t **init_vcache () {
	uint32_t num_tags = NUM_VCACHE_ENTRY * VCACHE_WAYS;
	uint8_t **packets = (uint8_t **) calloc(NUM_VCACHE * num_tags, sizeof(uint8_t *)); 
	for (int cache = 0; cache < NUM_VCACHE; cache++) {
		for (int tag = 0; tag < num_tags; tag++) {
			uint32_t addr = (1 << (ADDR_BIT-1)) | (tag << 4);  
			packets[(cache * num_tags) + tag] = get_pkt_12x12(addr, 0, cache, NUM_Y+1);
		}

	}
	return packets;
}

/*------------------------------------------------------------------------------*/
// Unfreeze tiles 
/*------------------------------------------------------------------------------*/

/*
 * only unfreezes (0, 0)
 * */
uint8_t **unfreeze_tiles () {
	uint8_t **packets = (uint8_t **) calloc(1, sizeof(uint8_t *)); 
	packets[0] = get_pkt_12x12((1 << 13), 0, 0, 0);
	return packets;
}

/*------------------------------------------------------------------------------*/
// Get packets for loadable segments of the binary
// This code is based on https://github.com/riscv/riscv-fesvr/blob/master/fesvr/elfloader.cc
/*------------------------------------------------------------------------------*/
uint8_t **text_pkts;
uint8_t **data_pkts;

uint32_t num_text_pkts;
uint32_t num_data_pkts;

void parse_elf (char *filename) { 	
	int fd = open(filename, O_RDONLY);
	struct stat s;
	assert(fd != -1);
	if (fstat(fd, &s) < 0)
		abort();
	size_t size = s.st_size;

	uint8_t* buf = (uint8_t*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  	assert(buf != MAP_FAILED);
  	close(fd);

	assert(size >= sizeof(Elf64_Ehdr));
	
	Elf32_Ehdr* eh = (Elf32_Ehdr *) buf;						 
	Elf32_Phdr* ph = (Elf32_Phdr *) (buf + eh->e_phoff); 
 	assert(size >= eh->e_phoff + eh->e_phnum*sizeof(*ph)); 
	
	uint32_t num_load = 0;
	// eh->e_phnum
//	printf("number of segments: %d\n", eh->e_phnum);
//	printf("segment 0 file size: %d\n", ph[0].p_memsz);
//	printf("segment 1 file size: %d\n", ph[1].p_memsz);

	
	num_text_pkts = 1 * (ph[TEXT].p_memsz / 4);
	text_pkts = (uint8_t **) calloc(num_text_pkts, sizeof(uint8_t *));
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
				for (int x = 0; x < 1; x++) { /* only computes for packets 0...131 */ 
					for (int ofs = 0; ofs < ph[i].p_memsz; ofs += 4) {
						int32_t addr = (ofs) >> 2; // ph[i].p_paddr
						uint32_t data = *((uint32_t *) (instructions + ofs));
						//printf("calling make packet with data=%x\n", data);
						text_pkts[x * (ph[i].p_memsz/4) + (ofs/4)] = get_pkt_12x12(addr, data, x, NUM_Y+1);
					}
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
					data_pkts[ofs/4] = get_pkt_12x12(addr, data, 0, 0);
				}
			}
			
		}
			
  	}
	munmap(buf, size);
} 







































