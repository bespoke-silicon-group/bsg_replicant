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

	/* packets 6-8 are first 24b of address */
	packet[6] = get_bits(addr, 0, 8);
	packet[7] = get_bits(addr, 8, 8);
	packet[8] = get_bits(addr, 16, 8);

	/* byte 9 = MSB {0s, 2 MSB of address} LSB */
	packet[9] = get_bits(addr, 24, 2);

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
		packets[idx] = get_pkt(icache_addr, 0, 0, 0);
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
			packets[(cache * num_tags) + tag] = get_pkt(addr, 0, cache, 5);
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
	packets[0] = get_pkt((1 << 13), 0, 0, 0);
	return packets;
}

/*------------------------------------------------------------------------------*/
// Get packets for loadable segments of the binary
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
						text_pkts[x * (ph[i].p_memsz/4) + (ofs/4)] = get_pkt(addr, data, x, 5);
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
					data_pkts[ofs/4] = get_pkt(addr, data, 0, 0);
				}
			}
			
		}
			
  	}
	munmap(buf, size);
} 
















































//vector<char *> get_segment (vector<char> data, uint32_t base_addr, uint32_t dest_x, uint32_t dest_y) {
//	vector<char *> packets;
//	
//
//	for (int ofs = 0; ofs < data.size(); ofs += (DATA_BIT / CHAR_BIT)) {
//		char *packet = (char *) calloc(MANYCORE_BIT / CHAR_BIT + 1, sizeof(char));
//		uint32_t addr = (base_addr + ofs) >> 2; 	
//		if (dest_x == DRAM_X && dest_y == DRAM_Y)
//			cout << "Address: " << addr << endl;
//		/* packet format: MSB {addr, op, op_ex, data, src_y_cord, src_x_cord, y_cord, x_cord) LSB */
//
//		/* byte 0: LSB {x_cord, y_cord, src_x_cord, src_y_cord} MSB */
//		uint8_t byte = dest_x + (dest_y << X_BIT) + (MY_X << (X_BIT + Y_BIT)) + (MY_Y << (X_BIT + Y_BIT + X_BIT)); 
//		packet[0] = reverse(byte);
//
//		/* bytes 1-4: data  */
//		for (int i= 0; i < (DATA_BIT / CHAR_BIT); i++) {
//			byte = (data[ofs + i]);
//			//cout << "data: " << hex << (int) (data[ofs + i] & 0xFF) << endl;
//			packet[1 + i] = reverse(byte);
//		}
//
//		/* byte 5: LSB {op_ex, op,  addr[0:1]} MSB */
//		byte = OP_EX + (ePacketOp_remote_store << OP_EX_BIT) + (get_bits(addr, 0, 2) << (OP_EX_BIT + OP_BIT));
//		// byte = get_bits(addr, 0, 2) + (OP_EX << 2) + (ePacketOp_remote_store << (2 + OP_EX_BIT));
//		packet[5] = reverse(byte);
//
//		/* byte 6: LSB {addr[2:9]} = {addr[2:7], addr[8:9]]} MSB */
//		byte = get_bits(addr, 2, 6) + (get_bits(addr, 8, 2) << 6);
//		// cout << "byte 6: " << hex << (int) (byte & 0xFF) << endl;
//		// byte = get_bits(addr, 8, 2) + (get_bits(addr, 2, 6) << 2);
//		packet[6] = reverse(byte);
//	
//		/* byte 8: LSB {addr[10:17]} = {addr[10:15], addr[16:17]} MSB */
//		byte = get_bits(addr, 10, 6) + (get_bits(addr, 16, 2) << 6);
//		// cout << "byte 7: " << hex << (int) (byte & 0xFF) << endl;
//		// byte = get_bits(addr, 16, 2) + (get_bits(addr, 10, 6) << 2);
//		packet[7] = reverse(byte);
//
//		/* byte 8: LSB {addr[18:26]} MSB */
//		byte = get_bits(addr, 18, 6) + (get_bits(addr, 24, 2) << 6);
//		// byte = get_bits(addr, 24, 2) + (get_bits(addr, 18, 6) << 2);
//		packet[8] = reverse(byte);
//		
//		/* byte 9: LSB {addr[27:30], 4 0s} MSB */
//		byte = get_bits(addr, 27, 4);
//		packet[9] = reverse(byte);
//		
//		packets.push_back(packet);
//	}
//	return packets;
//} 
//
//vector<vector<vector<char *>>> get_packets (string filename) { 	
//	int fd = open(filename.c_str(), O_RDONLY);
//	struct stat s;
//	assert(fd != -1);
//	if (fstat(fd, &s) < 0)
//		abort();
//	size_t size = s.st_size;
//
//
//	char* buf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
//  	assert(buf != MAP_FAILED);
//  	close(fd);
//
//	assert(size >= sizeof(Elf64_Ehdr));
//	
//	std::map<std::string, uint64_t> symbols;
//
//	Elf32_Ehdr* eh = (Elf32_Ehdr *) buf;						 
//	Elf32_Phdr* ph = (Elf32_Phdr *) (buf + eh->e_phoff); 
// 	assert(size >= eh->e_phoff + eh->e_phnum*sizeof(*ph)); 
//	
//	vector<vector<vector<char *>>> packets(eh->e_phnum);
//  
//	uint32_t num_load = 0;
//	// eh->e_phnum
//	for (unsigned i = 0; i < eh->e_phnum; i++) { 
//		if(ph[i].p_type == PT_LOAD && ph[i].p_memsz) { 
//			if (i == TEXT) { 
//				vector<char> data(ph[i].p_memsz, 0);
//				if (ph[i].p_filesz) { 
//        				assert(size >= ph[i].p_offset + ph[i].p_filesz);  
//					for (int byte = 0; byte < ph[i].p_filesz; byte++)
//						data[byte] = buf[ph[i].p_offset + byte];
//				}			
//				for (int row = 0; row < NUM_Y-1; row++) {
//					for (int col = 0; col < NUM_X; col++) {
//						vector<char *> segment = get_segment(data, ph[i].p_paddr, col, row);
//						packets[TEXT].push_back(segment);
//					}
//				}
//			}
//			else if (i == DATA) {
//				vector<char> data(ph[i].p_memsz, 0);
//				if (ph[i].p_filesz) { 
//        				assert(size >= ph[i].p_offset + ph[i].p_filesz);  
//					for (int byte = 0; byte < ph[i].p_filesz; byte++)
//						data[byte] = buf[ph[i].p_offset + byte];
//				}		
//
//				vector<char *> segment = get_segment(data, ph[i].p_paddr, DRAM_X, DRAM_Y);
//				packets[DATA].push_back(segment); 			
//			}
//		} 
//  	}
//	munmap(buf, size);
//	return packets;
//} 
//
//

