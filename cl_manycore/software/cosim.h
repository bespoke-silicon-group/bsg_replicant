#ifndef COSIM_H
#define COSIM_H


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "fpga_pci_sv.h"
#include <utils/sh_dpi_tasks.h>

#include "deploy/fifo.h"
#include "deploy/loader/spmd_loader.h"

uint8_t *ocl_base = 0;

/**/
uint32_t *cosim_read_packet (uint8_t n) {
    int rc;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
   	uint32_t occupancy, receive_length;
	uint32_t *receive_packet = (uint32_t *) calloc(4, sizeof(uint32_t));	

	/* wait until read fifo has packets */
	do {
		printf("read(): checking occupancy of read fifo.\n");
		rc = fpga_pci_peek(pci_bar_handle, fifo[n][FIFO_OCCUPANCY], &occupancy);
		if (rc) {
			printf("Unable to peek!\n");
		}
	} while (occupancy < 1);
	
	/* read back the packet */
	rc = fpga_pci_peek(pci_bar_handle, fifo[n][FIFO_RECEIVE_LENGTH], &receive_length);
	if (receive_length == 16) { /* read back only if receive length is 16B */
		printf("fifo receive length is as expected.\n");
		for (int i = 0; i < 4; i++) {
			rc = fpga_pci_peek(pci_bar_handle, fifo[n][FIFO_READ], &receive_packet[i]);
			if (rc) {
				printf("Warning: could not peek dw %d of the receive packet.", i);
			}
		}
	}
	else {
		printf("fifo receive length is %d instead of 16 B. Not going to read from FIFO\n", receive_length);
	}
	
	return receive_packet;
}
bool cosim_write_packet(uint8_t n, uint8_t *packet) {
    int rc;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
	uint32_t readback;
	uint32_t *dw = (uint32_t *) packet;

	if (n >= NUM_FIFO) {
		printf("invalid fifo.\n");
		return false;
	}

	/* check vacancy */
    rc = fpga_pci_peek(pci_bar_handle, fifo[n][FIFO_VACANCY], &readback);
    if (rc) {
		printf("Unable to peek!\n");
		return false;
	}
	if (readback < 4) {
		printf("not enough space in fifo.\n");
		return false;
	}
	uint32_t init_vacancy = readback;

	/* write data */
	for (int i = 0; i < 4; i++) {
    	rc = fpga_pci_poke(pci_bar_handle, fifo[n][FIFO_WRITE], dw[i]);
		if (rc) {
			printf("Unable to poke.\n");
			return false;
		}	
	}

	/* write to transmit length reg */
	do {
	 	rc = fpga_pci_poke(pci_bar_handle, fifo[n][FIFO_TRANSMIT_LENGTH], 16);
		if (rc) {
			printf("Unable to poke.\n");	
			return false;
		}
    	rc = fpga_pci_peek(pci_bar_handle, fifo[n][FIFO_VACANCY], &readback);
		if (rc) {
			printf("Unable to peek.\n");
			return false;
		}
	} while (readback != init_vacancy);

	return true;
}


uint32_t cosim_get_host_credits () {
	pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
	uint32_t credits = 0;
    	uint32_t rc = fpga_pci_peek(pci_bar_handle, HOST_CREDITS, &credits);
	if (rc)
		printf("cosim_get_host_credits(): warning - could not read the device register.\n");
	printf("host credits: %u\n", credits);
	return credits;
}

bool all_host_req_complete() {
	return (cosim_get_host_credits() == MAX_CREDITS);
}

uint32_t cosim_get_recv_vacancy () {
	printf("cosim_get_recv_vacancy()\n");
	pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
	uint32_t vacancy = 0;
    	uint32_t rc = fpga_pci_peek(pci_bar_handle, 0x200, &vacancy);
	if (rc)
		printf("cosim_get_recv_vacancy(): warning - could not read the device register.\n");
	printf("vacancy is %d\n", vacancy);
	return vacancy;
}



bool cosim_can_read (uint32_t size) {
	return (cosim_get_recv_vacancy() >= size);
}

uint32_t cosim_check_dim () {
	pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
	uint32_t num_x = 0;
	uint32_t num_y = 0;

    	uint32_t rc = fpga_pci_peek(pci_bar_handle, MANYCORE_NUM_X, &num_x);
	if (rc)
		printf("cosim_get_host_credits(): warning - could not read the device register.\n");
    	rc = fpga_pci_peek(pci_bar_handle, MANYCORE_NUM_Y, &num_y);
	if (rc)
		printf("cosim_get_host_credits(): warning - could not read the device register.\n");
	return (NUM_X == num_x && NUM_Y == num_y);
}




/*! 
 * Copies data from Manycore to host.
 * @param buf array of Manycore packets that data should be copied into.
 * @param x destination x coordinate
 * @param y destination y coordinate
 * @param epa tile's physical address
 * @param size number of words to copy
 * @return whether or not transaction was successful
 * */
bool cosim_hb_epa_to_xeon_copy (uint32_t **buf, uint32_t x, uint32_t y, uint32_t epa, uint32_t size) {
	
	if (!cosim_can_read(size)) {
		printf("cosim_hb_xeon_to_copy(): not enough space to read.\n");
		return false;
	}
	uint8_t **packets = calloc(size, sizeof(uint8_t *));
	for (int i = 0; i < size; i++) {
		packets[i] = get_pkt(epa, 0, x, y, OP_REMOTE_LOAD);
	} 
	bool pass_requests = true; /* whether or not load requests send properly */
	for (int i = 0; i < size; i++) {
		if (!cosim_write_packet(0, packets[i])) {
			pass_requests = false;
			break;
		}
	}

	if (!pass_requests)
		printf("cosim_hb_xeon_to_epa_copy(): error when sending load request to Manycore.\n");

	/* read receive packets from Manycore. TODO: can result in infinite loop. */
	for (int i = 0; i < size; i++) {
		buf[i] = cosim_read_packet(0);
	}

	return pass_requests;
}

/*! 
 * Copies data from host to manycore
 * @param x destination x coordinate
 * @param y destination y coordinate
 * @param epa tile's physical address
 * @param size number of words to copy
 * @return whether or not transaction was successful
 * */
bool cosim_hb_xeon_to_epa_copy(uint32_t x, uint32_t y, uint32_t epa, uint32_t *buf, uint32_t size) {
	uint8_t **packets = calloc(size, sizeof(uint8_t *));
	for (int i = 0; i < size; i++) {
		packets[i] = get_pkt(epa, buf[i], x, y, OP_REMOTE_STORE);
	} 
	
	bool pass = true;
	for (int i = 0; i < size; i++) {
		if (!cosim_write_packet(0, packets[i])) {
			pass = false;
			break;
		}
	}

	if (!pass)
		printf("hb_xeon_to_epa_copy(): error when writing to Manycore.\n");

	return pass;

}

/*!
 *  * writes the binary's instructions into (x,y)'s icache.
 *   * */
void cosim_load_icache() {
	int num_icache_packets = num_text_pkts;	
	bool pass = true;
	
	for (int i = 0; i < num_icache_packets; i++) {
		if (!cosim_write_packet(0, icache_pkts[i])) {
			pass = false;
			break;
		}
	}
	if (pass) 
		printf("cosim_load_icache(): icache init finished.\n");
	else
		printf("cosim_load_icache(): icache init failed.\n");
}

/*!
 *  * writes the binary's data into (x,y)'s dmem.
 *   * */
void cosim_load_dmem() {
	bool pass = true;
	
	for (int i = 0; i < num_data_pkts; i++) {
		if (!cosim_write_packet(0, data_pkts[i])) {
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
void cosim_load_dram() {
	printf("Loading DRAM with the binary's instructions ...\n");
	bool pass = true;

	for (int i = 0; i < num_text_pkts; i++) {
		if (!cosim_write_packet(0, text_pkts[i])) {
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
void cosim_unfreeze (uint8_t x, uint8_t y) {
	printf("Unfreezing tile (%d, %d).\n", x, y);
	uint8_t **unfreeze_pkts = unfreeze_tiles(x, y); 
	bool pass_unfreeze = true;
	if (!cosim_write_packet(0, unfreeze_pkts[0])) {
		pass_unfreeze = false;
	}
	if (pass_unfreeze)
		printf("unfreeze finished.\n");
	else
		printf("unfreeze failed.\n");	
}

void cosim_load_binary(char *path, uint8_t x, uint8_t y) {
	parse_elf(getenv(path), x, y, true);
    printf("cosim_load_binary(): Beginning to load program\n");
	cosim_load_icache(); /* load (x, y)'s icache with the binary's instructions */
	cosim_load_dram(); /* load DRAM with the binary's instructions */	
	cosim_load_dmem(); /* loads (x, y)'s dmem with the binary's data */	
	printf("cosim_load_binary(): Finished loading program.\n");
}

/*!
 * Writes data to a tile's DMEM and reads it back.  
 * */

void cosim_read_write_test () {
	if (!cosim_check_dim()) {
		printf("Manycore dimensions in FPGA are not as expected.\n");
		return;
	}
	
	/* store data in tile */
	uint32_t data = 0xABCDABCD;
	bool write = cosim_hb_xeon_to_epa_copy(0, 0, DMEM_BASE >> 2, &data, 1);

	if (!write) {
		printf("cosim_read_write_test(): write to tile failed.\n");
		return;
	}
	/* read back data */
	uint32_t **buf = (uint32_t **) calloc(1, sizeof(uint32_t *));
	bool read = cosim_hb_epa_to_xeon_copy (buf, 0, 0, DMEM_BASE >> 2, 1); 
	if (read) {
		printf("cosim_read_write_test(): read data is %u\n", get_data(buf[0]));
		print_hex((uint8_t *) buf[0]);
	}
	else {
		printf("cosim_read_write_test(): read from tile failed.\n");
	}
	
	// check that all host requests have been completed.
	if (!all_host_req_complete())
		printf("cosim_read_write_test(): warning - there are outstanding host requests.\n");
}

/*!
 * sends the binary located at the path contained in environment variable "MAIN_LOOPBACK" to the Manycore.
 * Then, prints the binary's result packet.
 * */
void cosim_test_run_binary () {
	if (!cosim_check_dim()) {
		printf("Manycore dimensions in FPGA are not as expected.\n");
		return;
	}
		cosim_load_binary("MAIN_LOOPBACK", 0, 0); /* load the binary */
	cosim_unfreeze(0, 0); /* unfreeze the tile */
	
	uint32_t *receive_packet = cosim_read_packet(1);
	printf("Receive packet: ");
	print_hex((uint8_t *) receive_packet);

	if (!all_host_req_complete())
		printf("cosim_test_run_binary(): warning - there are outstanding host requests.\n");
}


/*!
 * Uses 2 Manycore tiles to parallelize vector addition.
 * */

void cosim_vector_add () {
	uint32_t n = 10;
	uint32_t m = n / 2;
	uint32_t A_BASE = 4096;
	uint32_t B_BASE = 4096 + 64;
	uint32_t C_BASE = 4096 + 64 + 64;

	/* load binary into tiles (0,0), (0, 1), and DRAM */
	parse_elf(getenv("VEC_ADD"), 0, 0, true); /* TODO */
	printf("Number of instructions: %u, Size of data segment: %d words\n", num_text_pkts, num_data_pkts);
	cosim_load_icache(); 

	cosim_load_dmem();
	cosim_load_dram();
	parse_elf(getenv("VEC_ADD"), 1, 0, false);
	cosim_load_icache();
	cosim_load_dmem();
	
	/* load A and B vectors */
	srand(0);
	uint32_t zero = 0;
	for (int i = 0; i < n; i++) {
		uint32_t a_rand = rand();
		uint32_t b_rand = rand();
		cosim_hb_xeon_to_epa_copy(0, NUM_Y + 1, A_BASE + i, &a_rand, 1);
		cosim_hb_xeon_to_epa_copy(0, NUM_Y + 1, B_BASE + i, &b_rand, 1);
		cosim_hb_xeon_to_epa_copy(0, NUM_Y + 1, C_BASE + i, &zero, 1);
		// verify that A and B have been written to 
		uint32_t **a_read = calloc(1, sizeof(uint32_t *));
		uint32_t ** b_read = calloc(1, sizeof(uint32_t *));
		cosim_hb_epa_to_xeon_copy(a_read, 0, NUM_Y + 1, A_BASE + i, 1);
		cosim_hb_epa_to_xeon_copy(b_read, 0, NUM_Y + 1, B_BASE + i, 1);
		printf("a read: ");
		print_hex((uint8_t *) a_read[0]);	
		printf("b read: ");
		print_hex((uint8_t *) b_read[0]);
	}

	cosim_unfreeze(0, 0); /* unfreeze tiles */
	cosim_unfreeze(1, 0);	

	uint32_t **result = (uint32_t **) calloc(n, sizeof(uint32_t *));
	if (cosim_hb_epa_to_xeon_copy(result, 0, NUM_Y + 1, C_BASE, n) == false) { /* read back result from DRAM */
		printf("error reading back result from Manycore.\n");
		return;
	}	

	for (int i =  0; i < n; i++) { /* print result */
		uint32_t data = get_data(result[i]); /* TODO */
		printf("result[%d]: ", data);
		print_hex((uint8_t *) result[i]);
	}
}


#endif

