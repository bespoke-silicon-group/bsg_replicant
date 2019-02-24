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
#else
	#include "fpga_pci_sv.h"
	#include <utils/sh_dpi_tasks.h>
	#include "bsg_manycore_driver.h"
#endif

char *MANYCORE_DEVICE_PATH = "/dev/bsg_manycore_kernel_driver";

uint8_t NUM_Y = 4;


static bool hb_mc_check_fd (uint8_t fd) {
	return (fd_table[fd] != -1);
}

bool hb_mc_check_device (uint8_t fd) {
	#ifdef COSIM
		return true;
	#else
		return (hb_mc_check_fd(fd) && ocl_table[fd] != NULL);
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

/*
 * mmap's the OCL bar of the device.
 * */
static char *hb_mc_mmap_ocl (uint8_t fd) {
	char *addr = (char *) mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_table[fd], 0); 
	if (addr == MAP_FAILED) {
		printf("mmap_ocl(): mmap failed.\n");
		return 0;
	}
	ocl_table[fd] = addr;
	return addr;
} 

/* opens the device file and mmap's it. */
bool hb_mc_init_host (char *dev_path, uint8_t *fd) {
	int dev_fd = open(dev_path, O_RDWR);
	if (dev_fd == -1) {
		printf("Unable to open device.\n");
		return false; 
	}

	fd_table[num_dev] = dev_fd;
	*fd = num_dev;
	char *ocl_base = hb_mc_mmap_ocl(*fd);
	if (!ocl_base) {
		printf("hb_mc_init_host(): unable to mmap device.\n");
		return false;
	}	
	
	ocl_table[*fd] = ocl_base;
	num_dev++;
	return true; 
}

/*
 * unmaps the ocl BAR and closes the file. 
 * */
void hb_mc_close_host (uint8_t fd) {
	if (!hb_mc_check_device(fd)) {
		printf("close_host(): warning - device was never initialized.\n");
	}
	munmap(ocl_table[fd], 0x4000);
	close(fd_table[fd]);
}

/*!
 * checks if the dimensions of the Manycore matches with what is expected.
 * */
bool hb_mc_check_dim (uint8_t fd) {
	if (!hb_mc_check_device(fd)) {
		printf("check_dim(): device not initialized.\n");
		return false;
	}
	uint32_t num_x = hb_mc_read(fd, MANYCORE_NUM_X, 32);
	uint32_t num_y = hb_mc_read(fd, MANYCORE_NUM_Y, 32);
	return (NUM_X == num_y) && (NUM_Y == num_y);
}

/*
 * writes 128B to the nth fifo
 * returns true on success and false on failure.
 * */
bool hb_mc_write_fifo (uint8_t fd, uint8_t n, uint32_t *val) {
	if (n >= NUM_FIFO) {
		printf("write_fifo(): invalid fifo.\n");
		return false;
	}

	else if (!hb_mc_check_device(fd)) {
		printf("write_fifo(): device not initialized.\n");
		return false;
	}	
	
	uint16_t init_vacancy = hb_mc_read(fd, fifo[n][FIFO_VACANCY], 16);

	#ifdef DEBUG
	printf("write(): vacancy is %u\n", init_vacancy);	
	#endif

	if (init_vacancy < 4) {
		printf("not enough space in fifo.\n");
		return false;
	}
	printf("write_fifo(): init_vacancy = %u\n", init_vacancy);
	for (int i = 0; i < 4; i++) {
		hb_mc_write(fd, fifo[n][FIFO_WRITE], val[i], 32);
	}

	while (hb_mc_read(fd, fifo[n][FIFO_VACANCY], 16) != init_vacancy) {
		hb_mc_write(fd, fifo[n][FIFO_TRANSMIT_LENGTH], 16, 16);
	}
	return true;
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

	else if (!hb_mc_check_device(fd)) {
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

	else if (!hb_mc_check_device(fd)) {
		printf("clear_int(): device not initialized.\n");
		return;
	}		

	hb_mc_write(fd, fifo[n][FIFO_ISR], 0xFFFFFFFF, 32);
}

/*
 * returns 0 if device is unitialized
 * */
uint32_t hb_mc_get_host_credits (uint8_t fd) {
	if (!hb_mc_check_device(fd)) {
		printf("get_host_credits(): device not initialized.\n");
		return 0;
	}		

	return hb_mc_read(fd, HOST_CREDITS, 32);
}

/*!
 * returns true if device is not initialized.
 * */
bool hb_mc_all_host_req_complete(uint8_t fd) {
	if (!hb_mc_check_device(fd)) {
		printf("get_host_req_complete(): device not initialized.\n");
		return true;
	}		

	return (hb_mc_get_host_credits(fd) == MAX_CREDITS);
}

/*
 * returns 0 if device is unitialized
 * */
uint32_t hb_mc_get_recv_vacancy (uint8_t fd) {
	if (!hb_mc_check_device(fd)) {
		printf("get_recv_vacancy(): device not initialized.\n");
		return 0;
	}	
	return hb_mc_read(fd, HOST_RECV_VACANCY, 32);
}

/*!
 * returns false if device is not initialized.
 * */
bool hb_mc_can_read (uint8_t fd, uint32_t size) {
	if (!hb_mc_check_device(fd)) {
		printf("can_read(): device not initialized.\n");
		return false;
	}	
	return (hb_mc_get_recv_vacancy(fd) >= size);
}


