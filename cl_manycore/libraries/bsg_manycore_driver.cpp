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
	#include <bsg_manycore_memory_manager.h>
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
	#include "bsg_manycore_memory_manager.h"
	#include "bsg_manycore_elf.h"
	#include "bsg_manycore_mem.h"
#endif



static uint8_t NUM_Y = 0; /*! Number of rows in the Manycore. */
static uint8_t NUM_X = 0; /*! Number of columns in the Manycore. */

static uint32_t const DRAM_SIZE = 0x80000000;
static awsbwhal::MemoryManager *mem_manager[1] = {(awsbwhal::MemoryManager *) 0}; /* This array has an element for every EVA <-> NPA mapping. Currently, only one mapping is supported. */
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
 * Gets the x coordinates of a list of tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_x(tile_t *tiles, uint8_t *x_list, uint32_t num_tiles) {
	if (!tiles || !x_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		x_list[i] = tiles[i].x;
	}
	return HB_MC_SUCCESS;
}

/*!
 * Gets the x coordinates of a list of tile_t structs.
 * @param tiles array of tiles. Must be allocated by the caller.
 * @param x_list array of x coordinates. Must be allocated by the caller.
 * @param num_tiles array number of tiles.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL otherwise. 
 */
static int hb_mc_get_y(tile_t *tiles, uint8_t *y_list, uint32_t num_tiles) {
	if (!tiles || !y_list) {
		return HB_MC_FAIL;
	}
	for (int i = 0; i < num_tiles; i++) {
		y_list[i] = tiles[i].y;
	}
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

/*
 * creates a awsbwhal::MemoryManager object and stores it in a global table.
 * @param eva_id which specifies which EVA <-> NPA mapping.
 * @param elf path to ELF binary
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
static int hb_mc_create_memory_manager (eva_id_t eva_id, char *elf) {
	eva_t program_end;
	if (symbol_to_eva(elf, "_bsg_dram_end_addr", &program_end) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	uint32_t alignment = 32;
	uint32_t start = program_end + alignment - (program_end % alignment); /* start at the next aligned block */
	uint32_t size = DRAM_SIZE;
	mem_manager[eva_id] = new awsbwhal::MemoryManager(DRAM_SIZE, start, alignment); 
	return HB_MC_SUCCESS;	
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

/*
 * Initializes Manycore tiles so that they may run kernels.
 * @param fd userspace file descriptor, which must be obtained from hb_mc_init_host.
 * @param eva_id specifies what the EVA-NPA mapping is.
 * @param tiles an array of tile_t structs to initialize.
 * @param num_tiles the number of tiles to initialize.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_init_device (uint8_t fd, eva_id_t eva_id, char *elf, tile_t *tiles, uint32_t num_tiles) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* eva_id not supported */
	} 

	for (int i = 0; i < num_tiles; i++) { /* initialize tiles */
		hb_mc_freeze(fd, tiles[i].x, tiles[i].y);
		hb_mc_set_tile_group_origin(fd, tiles[i].x, tiles[i].y, tiles[i].origin_x, tiles[i].origin_y);
	}


	/* load the elf into each tile */
	uint8_t x_list[num_tiles], y_list[num_tiles];	
	hb_mc_get_x(tiles, &x_list[0], num_tiles);
	hb_mc_get_y(tiles, &y_list[0], num_tiles); 
	hb_mc_load_binary(fd, elf, &x_list[0], &y_list[0], num_tiles);
	/* create a memory manager object */
	if (hb_mc_create_memory_manager(eva_id, elf) != HB_MC_SUCCESS)
		return HB_MC_FAIL;
  	
	/* unfreeze the tile group */
	for (int i = 0; i < num_tiles; i++) {
		hb_mc_unfreeze(fd, tiles[i].x, tiles[i].y);
	}
	return HB_MC_SUCCESS;
}

/*!
 * Initializes Manycore tiles so that they may run kernels.
 * @param fd userspace file descriptor
 * @param eva_id specifies what the EVA-NPA mapping is.
 * @param tiles an array of tile_t structs to initialize.
 * @param num_tiles the number of tiles to initialize.
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_device_finish (uint8_t fd, eva_id_t eva_id, tile_t *tiles, uint32_t num_tiles) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* eva_id not supported */
	} 

	else if (!mem_manager[eva_id])
		return HB_MC_SUCCESS; /* there is no memory manager to deinitialize */
	
	delete(mem_manager[eva_id]);
	
	for (int i = 0; i < num_tiles; i++) { /* freeze tiles */
		hb_mc_freeze(fd, tiles[i].x, tiles[i].y);
	}

	return HB_MC_SUCCESS;
}
#ifdef COSIM
/*!
 * This function is for testing hb_mc_init_device() in cosimulation. 
 */
void _hb_mc_get_mem_manager_info(eva_id_t eva_id, uint32_t *start, uint32_t *size) {
	if (!mem_manager[eva_id]) {
		printf("_hb_mc_get_mem_manager_info(): mem manager not initialized.\n");
		return;
	}
	*start = mem_manager[eva_id]->start();
	*size =mem_manager[eva_id]->size();
}
#endif

/*!
 * allocates memory in Manycore
 *@param eva_id specifies EVA-NPA mapping.
 *@param size in bytes.
 *@return an EVA address. On failure, 0 will be returned. 
 */
eva_t hb_mc_device_malloc (eva_id_t eva_id, uint32_t size) {
	if (eva_id != 0) {
		return 0; /* invalid EVA ID */
	}
	else if (!mem_manager[eva_id]) {
		return 0; /* memory manager has not been initialized */
	}

	eva_t result = mem_manager[eva_id]->alloc(size);
	return result;
}

/*!
 * frees Hammerblade Manycore memory.
 *@param eva_id specifies EVA-NPA mapping.
 *@param eva address to free.
 *@return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. This function can fail if eva_id is invalid or of the memory manager corresponding to eva_id has not been initialized.
 */
int hb_mc_device_free (eva_id_t eva_id, eva_t eva) {
	if (eva_id != 0) {
		return HB_MC_FAIL; /* invalid EVA ID */
	}
	else if (!mem_manager[eva_id]) {
		return HB_MC_FAIL; /* memory manager has not been initialized */
	}

	mem_manager[eva_id]->free(eva);
	return HB_MC_SUCCESS;
}



/*
 * returns HB_MC_SUCCESS if eva is a global network address and HB_MC_FAIL if not.
 * */
static int hb_mc_is_global_network (eva_t eva) {
	if (hb_mc_get_bits(eva, 30, 2) == 0x1) 
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*
 * returns HB_MC_SUCCESS if eva is a DRAM address and HB_MC_FAIL if not.
 * */
static int hb_mc_eva_is_dram (eva_t eva) {
	if (hb_mc_get_bits(eva, 31, 1) == 0x1) 
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*!
 * checks if NPA is in DRAM.
 * */
static int hb_mc_npa_is_dram (npa_t *npa) {
	if (npa->y == (NUM_Y + 1))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;	
}
/*
 * returns x coordinate of a global network address.
 * */
static uint32_t hb_mc_global_network_get_x (eva_t eva) {
	return hb_mc_get_bits(eva, 18, 6); /* TODO: hardcoded */	
}

/*
 * returns y coordinate of a global network address.
 * */
static uint32_t hb_mc_global_network_get_y (eva_t eva) {
	return hb_mc_get_bits(eva, 24, 6); /* TODO: hardcoded */
}

/*
 * returns x coordinate of a DRAM address.
 * */
static uint32_t hb_mc_dram_get_x (eva_t eva) {
	return hb_mc_get_bits(eva, 29, 2); /* TODO: hardcoded */
}

/*
 * returns y coordinate of a DRAM address.
 * */
static uint32_t hb_mc_dram_get_y (eva_t eva) {
	return NUM_Y + 1;
}


/*
 * returns EPA of a global network address.
 * */
static uint32_t hb_mc_global_network_get_epa (eva_t eva) {
	return hb_mc_get_bits(eva, 0, 18) >> 2; /* TODO: hardcoded */ 
}

/*
 * returns EPA of a DRAM address.
 * */
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
static int hb_mc_valid_epa_vanilla (uint32_t epa) {
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
 * */
static int hb_mc_npa_is_valid (npa_t *npa) {
	int x_valid = (npa->x < NUM_X) ? HB_MC_SUCCESS : HB_MC_FAIL;
	int y_valid = (npa->y < NUM_Y || (npa->y == NUM_Y + 1)) ? HB_MC_SUCCESS : HB_MC_FAIL;
	
	if (!(x_valid == HB_MC_SUCCESS && y_valid == HB_MC_SUCCESS)) 
		return HB_MC_FAIL; /* invalid (x,y) */
	else if (hb_mc_npa_is_dram(npa) == HB_MC_SUCCESS && hb_mc_valid_epa_dram(npa->epa) == HB_MC_SUCCESS)
		return HB_MC_SUCCESS; /* valid DRAM NPA */
	else if (hb_mc_npa_is_dram(npa) != HB_MC_SUCCESS && hb_mc_valid_epa_vanilla(npa->epa) == HB_MC_SUCCESS)
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
	else { /* tile */
		*eva = hb_mc_npa_to_eva_global_remote(npa);
	}
	return HB_MC_SUCCESS;
}

/*
 * caller must ensure eva_id is valid. */
static int hb_mc_cpy_to_eva (uint8_t fd, eva_id_t eva_id, eva_t dst, uint32_t *src) {
	npa_t npa;	
	int error = hb_mc_eva_to_npa(eva_id, dst, &npa);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not convert EVA to an NPA */
	}
	error = hb_mc_copy_to_epa (fd, npa.x, npa.y, npa.epa, src, 1 /* 1 word */);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not send data to Manycore */
	}
	return HB_MC_SUCCESS;
}

/*
 * caller must esure eva_id is valid. 
 * */
static int hb_mc_cpy_from_eva (uint8_t fd, eva_id_t eva_id, hb_mc_response_packet_t *dest, eva_t src) {
	npa_t npa;	
	int error = hb_mc_eva_to_npa(eva_id, src, &npa);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not convert EVA to an NPA */
	}
	error = hb_mc_copy_from_epa (fd, dest, npa.x, npa.y, npa.epa, 1 /* 1 word */);
	if (error != HB_MC_SUCCESS) {
		return HB_MC_FAIL; /* could not send data to Manycore */
	}
	return HB_MC_SUCCESS;
}

int hb_mc_device_memcpy (uint8_t fd, eva_id_t eva_id, void *dst, const void *src, uint32_t count, enum hb_mc_memcpy_kind kind) {
	if (eva_id != 0) 
		return HB_MC_FAIL; /* invalid EVA ID */

	else if (kind == hb_mc_memcpy_to_device) { /* copy to Manycore */
		eva_t dst_eva = (eva_t) reinterpret_cast<uintptr_t>(dst);
		for (int i = 0; i < count; i += sizeof(uint32_t)) { /* copy one word at a time */
			char *src_word = (char *) src + i;
			int error = hb_mc_cpy_to_eva(fd, eva_id, dst_eva + i, (uint32_t *) (src_word)); 		
			if (error != HB_MC_SUCCESS)
				return HB_MC_FAIL; /* copy failed */
		}
		return HB_MC_SUCCESS;	
	}
	
	else if (kind == hb_mc_memcpy_to_host) { /* copy to Host */
		eva_t src_eva = (eva_t) reinterpret_cast<uintptr_t>(src);
		for (int i = 0; i < count; i += sizeof(uint32_t)) { /* copy one word at a time */
			hb_mc_response_packet_t *dst_packet = (hb_mc_response_packet_t *) dst + (i / sizeof(uint32_t));
			int error = hb_mc_cpy_from_eva(fd, eva_id, dst_packet, src_eva + i); 		
			if (error != HB_MC_SUCCESS)
				return HB_MC_FAIL; /* copy failed */
		}
		return HB_MC_SUCCESS;	
	}
	else 
		return HB_MC_FAIL; /* invalid kind */
}


void hb_mc_device_sync (uint8_t fd, hb_mc_request_packet_t *finish) {
	while (1) {
		hb_mc_request_packet_t recv;
		hb_mc_read_fifo(fd, 1, (hb_mc_packet_t *) &recv); /* wait for Manycore to send packet */
		
		if (hb_mc_request_packet_equals(&recv, finish) == HB_MC_SUCCESS) 
			break; /* finish packet received from Hammerblade Manycore */
	}	
}

typedef struct {
	uint32_t argc;
	uint32_t argv[];
} kernel_args_t;

int hb_mc_device_launch (eva_id_t eva_id, kernel_args_t *args, char *elf, tile_t *tile) {
	/* transfer the arguments to dram */
	size_t args_size = sizeof(args);
	eva_t args_p = hb_mc_device_malloc (eva_id, args_size); 
	
	/* write location of args to tile group */

	/* write return address */
	
	/* write kernel address */
	
	
}
