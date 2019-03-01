#ifndef COSIM
	#include <bsg_manycore_driver.h> /* TODO: should be angle brackets */ 
	#include <bsg_manycore_loader.h>
#else
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_loader.h"
#endif

uint32_t DMEM_BASE = 0x1000;

/*!
 *  * writes the binary's instructions into (x,y)'s icache.
 *   * */
static bool hb_mc_load_packets(uint8_t fd, uint8_t **pkts, uint32_t num_pkts) {
	bool pass = true;

	if (!hb_mc_check_device(fd)) {
		printf("load_packets(): warning - device was never initialized.\n");
		return false;
	}
	
	for (int i = 0; i < num_pkts; i++) {
		if (!hb_mc_write_fifo(fd, 0, (uint32_t *) pkts[i])) {
			pass = false;
			break;
		}
	}
	if (!pass)
		printf("load_packets(): load failed.\n");
	return pass;
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
						(*dram_pkts)[ofs/4] = hb_mc_get_pkt(addr, data, 0, NUM_Y+1, OP_REMOTE_STORE);
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
	if (!hb_mc_check_device(fd)) {
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
 * Returns an array of Manycore packets that should be used to freeze the needed tiles. Triggers a soft reset on the tile at (X, Y)
 * @return array of Manycore packets.
 * */

static uint8_t *hb_mc_get_freeze_pkt (uint8_t x, uint8_t y) {
	uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
	packet = hb_mc_get_pkt((1 << 13), 1, x, y, OP_REMOTE_STORE);
	return packet;
}

/*!
 *  * freezes (x,y).
 *   * */
void hb_mc_freeze (uint8_t fd, uint8_t x, uint8_t y) {
	if (!hb_mc_check_device(fd)) {
		printf("freeze(): warning - device was not initialized.\n");
		return;
	}
		
	printf("Freezing tile (%d, %d).\n", x, y);
	uint8_t *freeze_pkt = hb_mc_get_freeze_pkt(x, y); 
	bool pass_freeze = true;
	if (!hb_mc_write_fifo(fd, 0, (int *) freeze_pkt)) {
		pass_freeze = false;
	}
	if (pass_freeze)
		printf("freeze finished.\n");
	else
		printf("freeze failed.\n");	
}

/*!
 * Returns an array of Manycore packets that should be used to unfreeze the needed tiles. Currently, on the tile at (X, Y) = (0, 0) is unfrozen.
 * @return array of Manycore packets.
 * */

static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y) {
	uint8_t *packet = (uint8_t *) calloc(16, sizeof(uint8_t)); 
	packet = hb_mc_get_pkt((1 << 13), 0, x, y, OP_REMOTE_STORE);
	return packet;
}

/*!
 *  * unfreezes (x,y).
 *   * */
void hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y) {
	if (!hb_mc_check_device(fd)) {
		printf("unfreeze(): warning - device was not initialized.\n");
		return;
	}
		
	printf("Unfreezing tile (%d, %d).\n", x, y);
	uint8_t *unfreeze_pkt = hb_mc_get_unfreeze_pkt(x, y); 
	bool pass_unfreeze = true;
	if (!hb_mc_write_fifo(fd, 0, (int *) unfreeze_pkt)) {
		pass_unfreeze = false;
	}
	if (pass_unfreeze)
		printf("unfreeze finished.\n");
	else
		printf("unfreeze failed.\n");	
}

