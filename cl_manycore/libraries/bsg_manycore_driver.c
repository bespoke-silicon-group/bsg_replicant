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

#ifndef COSIM
	#include <bsg_manycore_driver.h> /* TODO: should be angle brackets */ 
	#include <bsg_manycore_loader.h>
	#include <bsg_manycore_errno.h> 
	#include <fpga_pci.h>
	#include <fpga_mgmt.h>
#else
	#include "fpga_pci_sv.h"
	#include <utils/sh_dpi_tasks.h>
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_loader.h"
 	#include "bsg_manycore_errno.h"
#endif



static uint8_t NUM_Y = 0; /*! Number of rows in the Manycore. */
static uint8_t NUM_X = 0; /*! Number of columns in the Manycore. */

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
	printf("map address is %p\n", ocl_table[fd]);
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
		printf("hb_mc_init_host(): unable to mmap device.\n");
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
		printf("hb_mc_check_dim(): device not initialized.\n");
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
int hb_mc_write_fifo (uint8_t fd, uint8_t n, packet_t *packet) {
	if (n >= NUM_FIFO) {
		printf("write_fifo(): invalid fifo.\n");
		return HB_MC_FAIL;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("write_fifo(): device not initialized.\n");
		return HB_MC_FAIL;
	}	
	
	uint16_t init_vacancy = hb_mc_read16(fd, fifo[n][FIFO_VACANCY]);

	#ifdef DEBUG
	printf("write(): vacancy is %u\n", init_vacancy);	
	#endif

	if (init_vacancy < 4) {
		printf("not enough space in fifo.\n");
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
 * returns dequeued element on success and INT_MAX on failure.
 * */
int hb_mc_read_fifo (uint8_t fd, uint8_t n, request_packet_t *buf) {
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
	printf("read(): read the receive length register @ %u to be %u\n", fifo[n][FIFO_RECEIVE_LENGTH], receive_length);
	#endif

	uint32_t buf_words[4]; /* 4 32b words make up a 128b Manycore packet */
	for (int i = 0; i < 4; i++) {
		buf_words[i] = hb_mc_read32(fd, fifo[n][FIFO_READ]);
	}
	
	*buf = *((request_packet_t *) (&buf_words[0]));  
}

/* clears interrupts for the nth fifo */
void hb_mc_clear_int (uint8_t fd, uint8_t n) {
	if (n >= NUM_FIFO) { 
		printf("Invalid fifo.\n");
		return;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("clear_int(): device not initialized.\n");
		return;
	}		

	hb_mc_write32(fd, fifo[n][FIFO_ISR], 0xFFFFFFFF);
}

/*
 * returns 0 if device is unitialized
 * */
uint32_t hb_mc_get_host_credits (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("get_host_credits(): device not initialized.\n");
		return 0;
	}		

	return hb_mc_read32(fd, HOST_CREDITS);
}

/*!
 * Checks that all host requests have been completed.
 * @return HB_MC_SUCCESS if all requests have been completed and HB_MC_FAIL otherwise.
 * */
int hb_mc_all_host_req_complete(uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("get_host_req_complete(): device not initialized.\n");
		return HB_MC_FAIL;
	}		
	if (hb_mc_get_host_credits(fd) == MAX_CREDITS)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;

}		

/*
 * returns 0 if device is unitialized
 * */
uint32_t hb_mc_get_recv_vacancy (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("get_recv_vacancy(): device not initialized.\n");
		return 0;
	}	
	return hb_mc_read32(fd, HOST_RECV_VACANCY);
}

/*!
 * @return HB_MC_SUCCESS if the HOST_RECV_VACANCY is at least of value SIZE and HB_MC_FAIL otherwise.
 * */
int hb_mc_can_read (uint8_t fd, uint32_t size) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("can_read(): device not initialized.\n");
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
 * Formatss a Manycore request packet.
 * @param packet packet struct that this function will populate. caller must allocate. 
 * @param addr address to send packet to.
 * @param data packet's data
 * @param x destination tile's x coordinate
 * @param y destination tile's y coordinate
 * @param opcode operation type (e.g load, store, etc.)
 * @return array of bytes that form the Manycore packet.
 * assumes all fields are <= 32
 * */
void hb_mc_format_request_packet(request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode) {

	request_packet_set_x_dst(packet, x);
	request_packet_set_y_dst(packet, y);	
	request_packet_set_x_src(packet, MY_X);
	request_packet_set_y_src(packet, MY_Y);
	request_packet_set_data(packet, data);
	request_packet_set_op_ex(packet, 0xF);
	request_packet_set_op(packet, opcode);	
	request_packet_set_addr(packet, addr);		

}

