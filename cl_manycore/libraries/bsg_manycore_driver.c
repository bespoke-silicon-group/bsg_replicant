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
	#include <bsg_manycore_errno.h> 
	#include <fpga_pci.h>
	#include <fpga_mgmt.h>
#else
	#include "fpga_pci_sv.h"
	#include <utils/sh_dpi_tasks.h>
	#include "bsg_manycore_driver.h"
 	#include "bsg_manycore_errno.h"
#endif

uint8_t NUM_Y = 4;

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

/*! 
 * caller must verify that fd is correct. */
static void hb_mc_write (uint8_t fd, uint32_t ofs, uint32_t val, uint8_t reg_size) {
	#ifdef COSIM
		fpga_pci_poke(PCI_BAR_HANDLE_INIT, ofs, val);
	#else
		char *ocl_base = ocl_table[fd];
		if (reg_size == 16)
			*((uint16_t *) (ocl_base + ofs)) = val;
		else
			*((uint32_t *) (ocl_base + ofs)) = val;
	#endif
}

/*! 
 * caller must verify that fd is correct. */
static uint32_t hb_mc_read (uint8_t fd, uint32_t ofs, uint8_t reg_size) {
	#ifdef COSIM
		uint32_t read;
		fpga_pci_peek(PCI_BAR_HANDLE_INIT, ofs, &read);
		return read;
	#else
		char *ocl_base = ocl_table[fd];
		if (reg_size == 16)
			return *((uint16_t *) (ocl_base + ofs));
		else
			return *((uint32_t *) (ocl_base + ofs));
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

/*! 
 * Initializes the FPGA at slot 0. 
 * Maps the FPGA to userspace and then creates a userspace file descriptor for it.  
 * @param fd pointer to which the userspace file descriptor is assigned. 
 * @return HB_MC_SUCCESS if device has been initialized and HB_MC_FAIL otherwise.
 * */
int hb_mc_init_host (uint8_t *fd) {
	*fd = num_dev;
	char *ocl_base = hb_mc_mmap_ocl(*fd);
	if (!ocl_base) {
		printf("hb_mc_init_host(): unable to mmap device.\n");
		return HB_MC_FAIL;
	}	
	
	ocl_table[*fd] = ocl_base;
	num_dev++;
	return HB_MC_SUCCESS; 
}
#endif

/*!
 * Checks if the dimensions of the Manycore matches with what is expected.
 * @return HB_MC_SUCCESS if its able to verify that the device has the expected dimensions and HB_MC_FAIL otherwise.
 * */
int hb_mc_check_dim (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("hb_mc_check_dim(): device not initialized.\n");
		return HB_MC_FAIL;
	}
	uint32_t num_x = hb_mc_read(fd, MANYCORE_NUM_X, 32);
	uint32_t num_y = hb_mc_read(fd, MANYCORE_NUM_Y, 32);
	if ((NUM_X == num_y) && (NUM_Y == num_y))
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}

/*
 * Writes 128B to the nth fifo
 * @return HB_MC_SUCCESS  on success and HB_MC_FAIL on failure.
 * */
int hb_mc_write_fifo (uint8_t fd, uint8_t n, uint32_t *val) {
	if (n >= NUM_FIFO) {
		printf("write_fifo(): invalid fifo.\n");
		return HB_MC_FAIL;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("write_fifo(): device not initialized.\n");
		return HB_MC_FAIL;
	}	
	
	uint16_t init_vacancy = hb_mc_read(fd, fifo[n][FIFO_VACANCY], 16);

	#ifdef DEBUG
	printf("write(): vacancy is %u\n", init_vacancy);	
	#endif

	if (init_vacancy < 4) {
		printf("not enough space in fifo.\n");
		return HB_MC_FAIL;
	}
	printf("write_fifo(): init_vacancy = %u\n", init_vacancy);
	for (int i = 0; i < 4; i++) {
		hb_mc_write(fd, fifo[n][FIFO_WRITE], val[i], 32);
	}

	while (hb_mc_read(fd, fifo[n][FIFO_VACANCY], 16) != init_vacancy) {
		hb_mc_write(fd, fifo[n][FIFO_TRANSMIT_LENGTH], 16, 16);
	}
	return HB_MC_SUCCESS;
}

/*
 * reads 128B from the nth fifo
 * returns dequeued element on success and INT_MAX on failure.
 * */
uint32_t *hb_mc_read_fifo (uint8_t fd, uint8_t n, uint32_t *val) {
	if (n >= NUM_FIFO) {
		printf("Invalid fifo.\n.");
		return NULL;
	}

	else if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("read_fifo(): device not initialized.\n");
		return NULL;
	}		

	while (hb_mc_read(fd, fifo[n][FIFO_OCCUPANCY], 16) < 1) {}

	uint32_t receive_length = hb_mc_read(fd, fifo[n][FIFO_RECEIVE_LENGTH], 16);
	if (receive_length != 16) {
		printf("read_fifo(): receive length of %d instead of 16.\n", receive_length);
		return NULL;
	}
	
	#ifdef DEBUG
	printf("read(): read the receive length register @ %u to be %u\n", fifo[n][FIFO_RECEIVE_LENGTH], receive_length);
	#endif

	if (!val){
		val = (int *) calloc(4, sizeof(int));
	}
	for (int i = 0; i < 4; i++) {
		val[i] = hb_mc_read(fd, fifo[n][FIFO_READ], 32);
	}

	return val;
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

	hb_mc_write(fd, fifo[n][FIFO_ISR], 0xFFFFFFFF, 32);
}

/*
 * returns 0 if device is unitialized
 * */
uint32_t hb_mc_get_host_credits (uint8_t fd) {
	if (hb_mc_check_device(fd) != HB_MC_SUCCESS) {
		printf("get_host_credits(): device not initialized.\n");
		return 0;
	}		

	return hb_mc_read(fd, HOST_CREDITS, 32);
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
	return hb_mc_read(fd, HOST_RECV_VACANCY, 32);
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
