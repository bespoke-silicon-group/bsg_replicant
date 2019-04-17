#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifndef COSIM
	#include <bsg_manycore_driver.h> /* TODO: should be angle brackets */ 
	#include <bsg_manycore_loader.h>
	#include <bsg_manycore_errno.h> 
	#include <bsg_manycore_elf.h>
	#include <bsg_manycore_mem.h>
	#include <fpga_pci.h>
	#include <fpga_mgmt.h>
#else
	#include "fpga_pci_sv.h"
	#include <utils/sh_dpi_tasks.h>
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_loader.h"
	#include "bsg_manycore_errno.h"
	#include "bsg_manycore_elf.h"
	#include "bsg_manycore_mem.h"
#endif



static uint8_t NUM_Y = 0; /*! Number of rows in the Manycore. */
static uint8_t NUM_X = 0; /*! Number of columns in the Manycore. */

int hb_mc_npa_to_eva (eva_id_t eva_id, npa_t *npa, eva_t *eva); 
/*!
 * writes to a 16b register in the OCL BAR of the FPGA
 * @param fd userspace file descriptor
 * @param ofs offset in OCL BAR to write to
 * @param val value to write 
 * caller must verify that fd is correct. */
static void hb_mc_write16 (uint8_t fd, uint32_t ofs, uint16_t val) {
	#ifdef COSIM
		fpga_pci_poke(PCI_BAR_HANDLE_INIT, ofs, val);
	#else
		char *ocl_base = ocl_table[fd];
		*((uint16_t *) (ocl_base + ofs)) = val;
	#endif
}

/*!
 * writes to a 32b register in the OCL BAR of the FPGA
 * @param fd userspace file descriptor
 * @param ofs offset in OCL BAR to write to
 * @param val value to write 
 * caller must verify that fd is correct. */
static void hb_mc_write32 (uint8_t fd, uint32_t ofs, uint32_t val) {
	#ifdef COSIM
		fpga_pci_poke(PCI_BAR_HANDLE_INIT, ofs, val);
	#else
		char *ocl_base = ocl_table[fd];
		*((uint32_t *) (ocl_base + ofs)) = val;
	#endif
}

/*!
 * reads from a 16b register in the OCL BAR of the FPGA
 * @param fd userspace file descriptor
 * @param ofs offset in OCL BAR to write to
 * @return the value of the register
 * caller must verify that fd is correct. */
static uint16_t hb_mc_read16 (uint8_t fd, uint32_t ofs) {
	#ifdef COSIM
		uint32_t read;
		fpga_pci_peek(PCI_BAR_HANDLE_INIT, ofs, &read);
		return read;
	#else
		char *ocl_base = ocl_table[fd];
		return *((uint16_t *) (ocl_base + ofs));
	#endif
}

/*!
 * reads from a 32b register in the OCL BAR of the FPGA
 * @param fd userspace file descriptor
 * @param ofs offset in OCL BAR to write to
 * @return the value of the register
 * caller must verify that fd is correct. */
static uint32_t hb_mc_read32 (uint8_t fd, uint32_t ofs) {
	#ifdef COSIM
		uint32_t read;
		fpga_pci_peek(PCI_BAR_HANDLE_INIT, ofs, &read);
		return read;
	#else
		char *ocl_base = ocl_table[fd];
		return *((uint32_t *) (ocl_base + ofs));
	#endif
}
/*! 
 * Checks if corresponding FPGA has been memory mapped. 
 * @param fd userspace file descriptor
 * @return HB_MC_SUCCESS if device has been mapped and HB_MC_FAIL otherwise.
 * */
int hb_mc_check_device (uint8_t fd) {
	#ifdef COSIM
		return HB_MC_SUCCESS;
	#else
		if (ocl_table[fd] != NULL)
			return HB_MC_SUCCESS;
		else
			return HB_MC_FAIL;
	#endif
}




#ifndef COSIM
/*
 * mmap's the OCL bar of the device.
 * */
static char *hb_mc_mmap_ocl (uint8_t fd) {
	int slot_id = 0, pf_id = FPGA_APP_PF, write_combine = 0, bar_id = APP_PF_BAR0;
	pci_bar_handle_t handle;
	fpga_pci_attach(slot_id, pf_id, bar_id, write_combine, &handle);
	fpga_pci_get_address(handle, 0, 0x4, (void **) &ocl_table[fd]);	
	#ifdef DEBUG
	fprintf(stderr, "hb_mc_mmap_ocl(): map address is %p\n", ocl_table[fd]);
	#endif
	return ocl_table[fd];
} 

#endif
/*! 
 * Initializes the FPGA at slot 0. 
 * Maps the FPGA to userspace and then creates a userspace file descriptor for it.  
 * @param fd pointer to which the userspace file descriptor is assigned. 
 * @return HB_MC_SUCCESS if device has been initialized and HB_MC_FAIL otherwise.
 * */
int hb_mc_init_host (uint8_t *fd) {
	*fd = num_dev;
	char *ocl_base;
	#ifndef COSIM
	ocl_base = hb_mc_mmap_ocl(*fd);
	if (!ocl_base) {
		fprintf(stderr, "hb_mc_init_host(): unable to mmap device.\n");
		return HB_MC_FAIL;
	}	
	#else
	ocl_base = 0;
	#endif
	ocl_table[*fd] = ocl_base;
	num_dev++;

	/* initialize dimension variables */
	NUM_X = hb_mc_read32(*fd, MANYCORE_NUM_X);
	NUM_Y = hb_mc_read32(*fd, MANYCORE_NUM_Y);	

	return HB_MC_SUCCESS; 
}

/*!
 * Checks if the dimensions of the Manycore matches with what is expected.
 * @return HB_MC_SUCCESS if its able to verify that the device has the expected dimensions and HB_MC_FAIL otherwise.
 * */
int hb_mc_check_dim (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_check_dim(): device not initialized.\n");
		return HB_MC_FAIL;
	}
	uint32_t num_x = hb_mc_read32(fd, MANYCORE_NUM_X);
	uint32_t num_y = hb_mc_read32(fd, MANYCORE_NUM_Y);
	if ((NUM_X == num_y) && (NUM_Y == num_y))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*
 * Writes 128B to the nth fifo
 * @return HB_MC_SUCCESS  on success and HB_MC_FAIL on failure.
 * */
int hb_mc_write_fifo (uint8_t fd, uint8_t n, hb_mc_packet_t *packet) {
	if (n >= NUM_FIFO) {
		fprintf(stderr, "hb_mc_write_fifo(): invalid fifo.\n");
		return HB_MC_FAIL;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_write_fifo(): device not initialized.\n");
		return HB_MC_FAIL;
	}	
	
	uint16_t init_vacancy = hb_mc_read16(fd, fifo[n][FIFO_VACANCY]);

	if (init_vacancy < 4) {
		fprintf(stderr, "hb_mc_write_fifo(): not enough space in fifo.\n");
		return HB_MC_FAIL;
	}
	for (int i = 0; i < 4; i++) {
		hb_mc_write32(fd, fifo[n][FIFO_WRITE], packet->words[i]);
	}

	while (hb_mc_read16(fd, fifo[n][FIFO_VACANCY]) != init_vacancy) {
		hb_mc_write16(fd, fifo[n][FIFO_TRANSMIT_LENGTH], 16);
	}
	return HB_MC_SUCCESS;
}

/*!
 * gets the occupancy of a PCIe FIFO.
 * @param[in] fd userspace file descriptor
 * @param[in] n which FIFO
 * @param[out] occupancy_p will be set to the occupancy of the fifo
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure
 * */
int hb_mc_get_fifo_occupancy (uint8_t fd, uint8_t n, uint32_t *occupancy_p) {
	if (n >= NUM_FIFO)
		return HB_MC_FAIL;
	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}		
	*occupancy_p = hb_mc_read16(fd, fifo[n][FIFO_OCCUPANCY]);
	return HB_MC_SUCCESS;
}


/*
 * reads 128B from the nth fifo
 * returns HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * */
int hb_mc_read_fifo (uint8_t fd, uint8_t n, hb_mc_packet_t *packet) {
	if (n >= NUM_FIFO) {
		return HB_MC_FAIL;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}		

	while (hb_mc_read16(fd, fifo[n][FIFO_OCCUPANCY]) < 1) {}

	uint16_t receive_length = hb_mc_read16(fd, fifo[n][FIFO_RECEIVE_LENGTH]);
	if (receive_length != 16) {
		return HB_MC_FAIL;
	}
	
	#ifdef DEBUG
	fprintf(stderr, "hb_mc_read_fifo(): read the receive length register @ %u to be %u\n", fifo[n][FIFO_RECEIVE_LENGTH], receive_length);
	#endif

	for (int i = 0; i < 4; i++) {
		packet->words[i] = hb_mc_read32(fd, fifo[n][FIFO_READ]);
	}

	return HB_MC_SUCCESS;
}

/* Clears interrupts for an AXI4-Lite FIFO.
 * @param fd userspace file descriptor
 * @param n fifo ID
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_clear_int (uint8_t fd, uint8_t n) {
	if (n >= NUM_FIFO) { 
		fprintf(stderr, "hb_mc_clear_int(): Invalid fifo.\n");
		return HB_MC_FAIL;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_clear_int(): device not initialized.\n");
		return HB_MC_FAIL;
	}		

	hb_mc_write32(fd, fifo[n][FIFO_ISR], 0xFFFFFFFF);
	return HB_MC_SUCCESS;
}

/*
 * @param fd userspace file descriptor
 * @return number of host credits on success and HB_MC_FAIL on failure.
 */
int hb_mc_get_host_credits (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_host_credits(): device not initialized.\n");
		return HB_MC_FAIL;
	}		
	return hb_mc_read32(fd, HOST_CREDITS);
}

/*!
 * Checks that all host requests have been completed.
 * @return HB_MC_SUCCESS if all requests have been completed and HB_MC_FAIL otherwise.
 * */
int hb_mc_all_host_req_complete(uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_host_req_complete(): device not initialized.\n");
		return HB_MC_FAIL;
	}		
	if (hb_mc_get_host_credits(fd) == MAX_CREDITS)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;

}		

/*
 * @param fd userspace file descriptor
 * @return the receive vacancy of the FIFO on success and HB_MC_FAIL on failure.
 */
int hb_mc_get_recv_vacancy (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_recv_vacancy(): device not initialized.\n");
		return HB_MC_FAIL;
	}	
	return hb_mc_read32(fd, HOST_RECV_VACANCY);
}

/*!
 * @return HB_MC_SUCCESS if the HOST_RECV_VACANCY is at least of value SIZE and HB_MC_FAIL otherwise.
 * */
int hb_mc_can_read (uint8_t fd, uint32_t size) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_can_read(): device not initialized.\n");
		return HB_MC_FAIL;
	}	
	if (hb_mc_get_recv_vacancy(fd) >= size)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*!
 * @param fd user-level file descriptor.
 * @return the number of columns in the Manycore.
 * */
uint8_t hb_mc_get_num_x () {
	return NUM_X;
} 

/*!
 * @param fd user-level file descriptor.
 * @return the number of rows in the Manycore.
 * */
uint8_t hb_mc_get_num_y () {
	return NUM_Y;
}
/*
 * Formats a Manycore request packet.
 * @param packet packet struct that this function will populate. caller must allocate. 
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * @return array of bytes that form the Manycore packet.
 * assumes all fields are <= 32
 * */
void hb_mc_format_request_packet(hb_mc_request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode) {
	hb_mc_request_packet_set_x_dst(packet, x);
	hb_mc_request_packet_set_y_dst(packet, y);	
	hb_mc_request_packet_set_x_src(packet, MY_X);
	hb_mc_request_packet_set_y_src(packet, MY_Y);
	hb_mc_request_packet_set_data(packet, data);
	hb_mc_request_packet_set_op_ex(packet, 0xF);
	hb_mc_request_packet_set_op(packet, opcode);	
	hb_mc_request_packet_set_addr(packet, addr);		

}

/*!
 * returns HB_MC_SUCCESS if eva is a global network address and HB_MC_FAIL if not.
 */
static int hb_mc_is_global_network (eva_t eva) {
	if (hb_mc_get_bits(eva, 30, 2) == 0x1) 
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*!
 * returns HB_MC_SUCCESS if eva is a DRAM address and HB_MC_FAIL if not.
 */
static int hb_mc_eva_is_dram (eva_t eva) {
	if (hb_mc_get_bits(eva, 31, 1) == 0x1) 
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*!
 * checks if NPA is in DRAM.
 */
static int hb_mc_npa_is_dram (npa_t *npa) {
	if (npa->y == (NUM_Y + 1))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}

/*!
 * checks if NPA is in host endpoint.
 */
static int hb_mc_npa_is_host (npa_t *npa) {
	if (npa->y == 0 && npa->x == (NUM_X - 1))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}

/*!
 * checks if NPA is a tile.
 */
static int hb_mc_npa_is_tile (npa_t *npa) {
	if ((npa->y >= 1 && npa->y < NUM_Y) && (npa->x >= 0 && npa->x < NUM_X))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}

/*
 * returns x coordinate of a global network address.
 */
static uint32_t hb_mc_global_network_get_x (eva_t eva) {
	return hb_mc_get_bits(eva, 18, 6); /* TODO: hardcoded */	
}

/*
 * returns y coordinate of a global network address.
 */
static uint32_t hb_mc_global_network_get_y (eva_t eva) {
	return hb_mc_get_bits(eva, 24, 6); /* TODO: hardcoded */
}

/*
 * returns x coordinate of a DRAM address.
 */
static uint32_t hb_mc_dram_get_x (eva_t eva) {
	return hb_mc_get_bits(eva, 29, 2); /* TODO: hardcoded */
}

/*
 * returns y coordinate of a DRAM address.
 */
static uint32_t hb_mc_dram_get_y (eva_t eva) {
	return NUM_Y + 1;
}


/*
 * returns EPA of a global network address.
 */
static uint32_t hb_mc_global_network_get_epa (eva_t eva) {
	return hb_mc_get_bits(eva, 0, 18) >> 2; /* TODO: hardcoded */ 
}

/*
 * returns EPA of a DRAM address.
 */
static uint32_t hb_mc_dram_get_epa (eva_t eva) {
	return hb_mc_get_bits(eva, 2, 27); /* TODO: hardcoded */ 
}



/*!
 * Converts an EVA address to an NPA address.
 * @param eva_id specifies EVA-NPA mapping.
 * @param eva EVA address
 * @param npa pointer to npa_t object where NPA address should be stored.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * This function only supports DRAM and Global Network Address EVAs.
 */
int hb_mc_eva_to_npa (eva_id_t eva_id, eva_t eva, npa_t *npa) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid eva_id */
	}
	else if (hb_mc_is_global_network(eva) == HB_MC_SUCCESS) {
		uint32_t x = hb_mc_global_network_get_x(eva);
		uint32_t y = hb_mc_global_network_get_y(eva);
		uint32_t epa = hb_mc_global_network_get_epa(eva);
		*npa = {x, y, epa};
	}
	else if (hb_mc_eva_is_dram(eva) == HB_MC_SUCCESS) {
		uint32_t x = hb_mc_dram_get_x(eva);	
		uint32_t y = hb_mc_dram_get_y(eva);
		uint32_t epa = hb_mc_dram_get_epa(eva);
		*npa = {x, y, epa};
	}
	else {
		return HB_MC_FAIL; /* invalid EVA */
	}
	return HB_MC_SUCCESS;
}


/*!
 * Checks if a Vanilla Core EPA is valid.
 * @param epa.
 * @return HB_MC_SUCCESS if the EPA is valid and HB_MC_FAIL if the EPA is invalid.
 * */
static int hb_mc_valid_epa_tile (uint32_t epa) {
	if (epa >= 0x1000 && epa <= 0x1FFF) /* TODO: hardcoded */
		return HB_MC_SUCCESS; /* data memory */
	else if (epa >= 0x1000000 && epa <= 0x1FFEFFF)	/* TODO: hardcoded */
		return HB_MC_SUCCESS; /* instruction cache */
	else if (epa == 0x20000) /* TODO: hardcoded */
		return HB_MC_SUCCESS; /* FREEZE CSR */
	else if (epa == 0x20004) /* TODO: hardcoded */
		return HB_MC_SUCCESS; /* Tile Group Origin X Cord CSR */
	else if (epa == 0x20008) /* TODO: hardcoded */
		return HB_MC_FAIL; /* Tile Group Origin Y Cord CSR */
} 

/*!
 * Checks if a DRAM EPA is valid.
 * @param epa.
 * @return HB_MC_SUCCESS if the EPA is valid and HB_MC_FAIL if the EPA is invalid.
 * */
static int hb_mc_valid_epa_dram (uint32_t epa) {
	uint32_t dram_size = (1 << 27) - 1; /* TODO: hardcoded */
	uint32_t dram_size_words = dram_size >> 2;
	if (epa <= dram_size_words)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;

}

/*!
 * checks if NPA has valid (x,y) coordinates. 
 */
static int hb_mc_npa_is_valid (npa_t *npa) {
	if (hb_mc_npa_is_dram(npa) == HB_MC_SUCCESS && hb_mc_valid_epa_dram(npa->epa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* valid DRAM NPA */
	else if (hb_mc_npa_is_host(npa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* for now, we assume any EPA is valid for the host */
	else if (hb_mc_npa_is_tile(npa) == HB_MC_SUCCESS && hb_mc_valid_epa_tile(npa->epa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* valid Vanilla Core NPA */
	else 
		return HB_MC_FAIL;
}

/*! creates a NPA to DRAM EVA.
 * @param[in] npa Caller should ensure that this is valid.
 */
static eva_t hb_mc_npa_to_eva_dram(const npa_t *npa) {
	eva_t eva = 0;
	eva |= (npa->epa << 2);
	eva |= (npa->x << (2 + 27)); /* TODO: hardcoded */
	eva |= (1 << 31); /* TODO: hardcoded */
	return eva;
}

/*! converts NPA to Global Remote EVA.
 * @param[in] npa Caller should ensure that this is valid.
 */
static eva_t hb_mc_npa_to_eva_global_remote(const npa_t *npa) {
	eva_t eva = 0;
	eva |= (npa->epa << 2);
	eva |= (npa->x << 18); /* TODO: hardcoded */
	eva |= (npa->y << 24); /* TODO: hardcoded */
	eva |= (1 << 30); /* TODO: hardcoded */
	return eva;
}

/*!
 * Converts an NPA to an EVA. 
 * @param eva_id specified EVA-NPA mapping.
 * @param npa pointer to npa_t struct to convert.
 * @param eva pointer to an eva_t that this function should set.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. This function will fail if the NPA is invalid.
 */
int hb_mc_npa_to_eva (eva_id_t eva_id, npa_t *npa, eva_t *eva) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid eva_id */
	}
	else if (hb_mc_npa_is_valid(npa) != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* invalid NPA address*/
	}
	else if (hb_mc_npa_is_dram(npa) == HB_MC_SUCCESS) {
		*eva = hb_mc_npa_to_eva_dram(npa);
	}
	else { /* tile or host endpoint */
		*eva = hb_mc_npa_to_eva_global_remote(npa);
	}
	return HB_MC_SUCCESS;
}

void hb_mc_device_sync (uint8_t fd, hb_mc_request_packet_t *finish) {
	while (1) {
		hb_mc_request_packet_t recv;
		hb_mc_read_fifo(fd, 1, (hb_mc_packet_t *) &recv); /* wait for Manycore to send packet */
		
		if (hb_mc_request_packet_equals(&recv, finish) == HB_MC_SUCCESS) 
			break; /* finish packet received from Hammerblade Manycore */
	}	
}

/*!
 * creates a tile group with a specified origin
 * @param[out] tiles an array of tiles that will be set in row-order. This should be allocated by the caller
 * @param[out] the number of tiles in the tile group
 * @param[in] num_tiles_x the number of columns in the tile group
 * @param[in] num_tiles_y the number of rows in the tile group
 * @param[in] origin_x the x coordinate of the tile group's origin
 * @param[in] origin_y the y coordinate of the tile group's origin 
 * */
void create_tile_group(tile_t tiles[], uint32_t num_tiles_x, uint32_t num_tiles_y, uint32_t origin_x, uint32_t origin_y) {
	/* create the tile group */
	for (int i = 0; i < num_tiles_y; i++) {
		for (int j = 0; j < num_tiles_x; j++) {
			int index = i * num_tiles_x + j;
			tiles[index].x = j + origin_x; 
			tiles[index].y = i + origin_y;
			tiles[index].origin_x = origin_x;
			tiles[index].origin_y = origin_y;
		}
	}
}

