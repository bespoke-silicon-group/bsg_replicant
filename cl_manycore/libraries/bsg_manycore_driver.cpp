#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifndef COSIM
	#include <bsg_manycore_errno.h> 
	#include <bsg_manycore_driver.h>  
	#include <bsg_manycore_tile.h>
	#include <bsg_manycore_mmio.h>
	#include <fpga_pci.h>
	#include <fpga_mgmt.h>
#else
	#include "bsg_manycore_errno.h"
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_tile.h"
	#include "bsg_manycore_mmio.h"
	#include "fpga_pci_sv.h"
	#include <utils/sh_dpi_tasks.h>
#endif

/* The following values are cached by the API during initialization */
static uint8_t hb_mc_manycore_dim_x = 0; 
static uint8_t hb_mc_manycore_dim_y = 0; 
static uint8_t hb_mc_host_intf_coord_x = 0; /*! network X coordinate of the host  */
static uint8_t hb_mc_host_intf_coord_y = 0; /*! network Y coordinate of the host */

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
int hb_mc_fifo_check (uint8_t fd) {
	#ifdef COSIM
		return HB_MC_SUCCESS;
	#else
		if (ocl_table[fd] != NULL)
			return HB_MC_SUCCESS;
		else
			return HB_MC_FAIL;
	#endif
}

/* Clears interrupts for an AXI4-Lite FIFO.
 * @param fd userspace file descriptor
 * @param dir fifo direction 
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure. 
 */
int hb_mc_clear_int (uint8_t fd, hb_mc_direction_t dir) {
	uint32_t isr_addr;

	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_clear_int(): device not initialized.\n");
		return HB_MC_FAIL;
	}

	isr_addr = hb_mc_mmio_fifo_get_reg_addr(dir, HB_MC_MMIO_FIFO_ISR_OFFSET);

	hb_mc_write32(fd, isr_addr, 0xFFFFFFFF);
	return HB_MC_SUCCESS;
}

#ifndef COSIM
/*
 * mmap's the OCL bar of the device.
 * */
static char *hb_mc_mmap_ocl (uint8_t fd) {
	int slot_id = 0, pf_id = FPGA_APP_PF, write_combine = 0, bar_id = APP_PF_BAR0;
	pci_bar_handle_t handle;
	fpga_pci_attach(slot_id, pf_id, bar_id, write_combine, &handle);
	fpga_pci_get_address(handle, 0, 0x4000, (void **) &ocl_table[fd]);	
	#ifdef DEBUG
	fprintf(stderr, "hb_mc_mmap_ocl(): map address is %p\n", ocl_table[fd]);
	#endif
	return ocl_table[fd];
} 
#endif

int hb_mc_fifo_enable(uint8_t fd){
	uint32_t ier_addr_byte;
	ier_addr_byte = hb_mc_mmio_fifo_get_reg_addr(HB_MC_MMIO_FIFO_TO_HOST, HB_MC_MMIO_FIFO_IER_OFFSET);
	hb_mc_write32(fd, ier_addr_byte, (1<<HB_MC_MMIO_FIFO_IXR_TC_BIT));
		
	ier_addr_byte = hb_mc_mmio_fifo_get_reg_addr(HB_MC_MMIO_FIFO_TO_DEVICE, HB_MC_MMIO_FIFO_IER_OFFSET);
	hb_mc_write32(fd, ier_addr_byte, (1<<HB_MC_MMIO_FIFO_IXR_TC_BIT));

	return HB_MC_SUCCESS;
}

/*!
 * gets the occupancy of a AXI MM FIFO.
 * @param[in] fd userspace file descriptor
 * @param[in] dir FIFO Interface Direction (HB_MC_MMIO_FIFO_TO_HOST or
 * HB_MC_MMIO_FIFO_TO_DEVICE)
 * @param[out] occupancy_p will be set to the occupancy of the fifo
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure
 * */
int hb_mc_fifo_get_occupancy (uint8_t fd, hb_mc_fifo_rx_t type, uint32_t *occupancy_p) {
	uint32_t addr;
	
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	
	addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET);

	*occupancy_p = hb_mc_read16(fd, addr);
	return HB_MC_SUCCESS;
}

/*
 * Drains a fifo from all stale packets. 
 * @param[in] fd userspace file descriptor
 * @param[in] dir FIFO Direction (HB_MC_FIFO_TO_DEVICE, or HB_MC_FIFO_TO_HOST)
  * returns HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * */
int hb_mc_fifo_drain (uint8_t fd, hb_mc_fifo_rx_t type) {
	int rc;
	uint32_t occupancy; 
	hb_mc_request_packet_t recv;

	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_fifo_drain(): userspace file descriptor %d not valid.\n", fd);
		return HB_MC_FAIL;
	}

	//dir = hb_mc_get_rx_direction(type);	
	rc = hb_mc_fifo_get_occupancy(fd, type, &occupancy); 
	if (rc != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_fifo_drain(): failed to get fifo %d occupancy.\n", type); 
		return HB_MC_FAIL;
	}

	/* Read stale packets from fifo */
	for (int i = 0; i < occupancy; i++){
		rc = hb_mc_fifo_receive(fd, type, (hb_mc_packet_t *) &recv); 
		if (rc != HB_MC_SUCCESS) {
			fprintf(stderr, "hb_mc_fifo_drain(): failed to read packet from fifo %d.\n", type); 
			return HB_MC_FAIL;
		}
		#ifdef DEBUG
		fprintf(stderr, "Packet drained from fifo %d: src (%d,%d), dst (%d,%d), addr: 0x%x, data: 0x%x.\n", type, recv.x_src, recv.y_src, recv.x_dst, recv.y_dst, recv.addr, recv.data);  
		#endif
	}

	/* recheck occupancy to make sure all packets are drained. */
	rc = hb_mc_fifo_get_occupancy(fd, type, &occupancy); 
	if (rc != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_fifo_drain(): failed to get fifo %d occupancy.\n", type); 
		return HB_MC_FAIL;
	}

	if (occupancy > 0){
		fprintf(stderr, "hb_mc_fifo_drain(): failed to drain fifo %d even after reading all stale packets.\n", type); 
		return HB_MC_FAIL;
	}
	return HB_MC_SUCCESS;
}

/*!
 * Initializes a Vanilla Core Endpoint's victim cache tag.
 * @param[in] fd userspace file descriptor.
 * @param[in] x x coordinate of tile
 * @param[in] y y coordinate of tile
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_init_cache_tag(uint8_t fd, uint8_t x, uint8_t y) {
	hb_mc_packet_t tag;	
	uint32_t vcache_word_addr;

	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	vcache_word_addr = hb_mc_tile_epa_get_word_addr(HB_MC_VCACHE_EPA_BASE, HB_MC_VCACHE_EPA_TAG_OFFSET);
	hb_mc_format_request_packet(fd, &tag.request, vcache_word_addr, 0, x, y, HB_MC_PACKET_OP_REMOTE_STORE);
		
	for (int i = 0; i < 4; i++) {
		if (hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &tag) != HB_MC_SUCCESS) {	
			return HB_MC_FAIL;
		}
	}
	return HB_MC_SUCCESS;
}

/*! 
 * Initializes the FPGA at slot 0. 
 * Maps the FPGA to userspace and then creates a userspace file descriptor for it.  
 * @param fd pointer to which the userspace file descriptor is assigned. 
 * @return HB_MC_SUCCESS if device has been initialized and HB_MC_FAIL otherwise.
 */
	int hb_mc_fifo_init (uint8_t *fd) {
	int rc;
	uint32_t cfg;
	char *ocl_base;
	*fd = num_dev;
	#ifndef COSIM
	ocl_base = hb_mc_mmap_ocl(*fd);
	if (!ocl_base) {
		fprintf(stderr, "hb_mc_fifo_init(): unable to mmap device.\n");
		return HB_MC_FAIL;
	}	
	#else
	ocl_base = 0;
	#endif
	ocl_table[*fd] = ocl_base;
	num_dev++;

	hb_mc_fifo_enable(*fd);

	rc = hb_mc_fifo_drain(*fd, HB_MC_FIFO_RX_REQ); 
	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	hb_mc_clear_int(*fd, HB_MC_MMIO_FIFO_TO_DEVICE);

	rc = hb_mc_fifo_drain(*fd, HB_MC_FIFO_RX_RSP);
	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	hb_mc_clear_int(*fd, HB_MC_MMIO_FIFO_TO_HOST);

	/* get device information */
	rc = hb_mc_get_config(*fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &cfg);
	if(rc != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	hb_mc_host_intf_coord_x = cfg;

	rc = hb_mc_get_config(*fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &cfg);
	if(rc != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	hb_mc_host_intf_coord_y = cfg;

	rc = hb_mc_get_config(*fd, HB_MC_CONFIG_DEVICE_DIM_X, &cfg);
	if(rc != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	hb_mc_manycore_dim_x = cfg;

	if((hb_mc_manycore_dim_x <= 0) || (hb_mc_manycore_dim_x > 32)){
		fprintf(stderr, "hb_mc_fifo_init(): Questionable manycore X dimension: %d.\n", hb_mc_manycore_dim_x);
		return HB_MC_FAIL;
	}

	rc = hb_mc_get_config(*fd, HB_MC_CONFIG_DEVICE_DIM_Y, &cfg);
	if(rc != HB_MC_SUCCESS)
		return HB_MC_FAIL;
	hb_mc_manycore_dim_y = cfg;

	if((hb_mc_manycore_dim_y <= 0) || (hb_mc_manycore_dim_y > 32)){
		fprintf(stderr, "hb_mc_fifo_init(): Questionable manycore Y dimension: %d.\n", hb_mc_manycore_dim_y);
		return HB_MC_FAIL;
	}

	return HB_MC_SUCCESS; 
}

/*
 * Drains all fifos and terminates the host. 
 * @param [in] fd userspace file descriptor.
 * returns HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_fifo_finish(uint8_t fd) {
	int rc;
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	rc = hb_mc_fifo_drain(fd, HB_MC_FIFO_RX_REQ);
	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	rc = hb_mc_fifo_drain(fd, HB_MC_FIFO_RX_RSP); 
	if (rc != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}
	
	return HB_MC_SUCCESS;
}

/*
 * Set a bit in the IER/IXR register. Due to Xilinx implementaation, only
 * 1-valued bits take effect (so no pre-read and or is necessary)
 * @param[in] fd userspace file descriptor
 * @param[in] dir FIFO Direction (HB_MC_FIFO_TO_DEVICE, or HB_MC_FIFO_TO_HOST)
 * @param[out] packet Manycore packet to write
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * */
void hb_mc_fifo_set_ixr_bit(uint8_t fd, hb_mc_direction_t dir, uint32_t reg, uint32_t bit){
	uint64_t addr = hb_mc_mmio_fifo_get_reg_addr(dir, reg);
	hb_mc_write32(fd, addr, (1<<bit));
}

/*
 * Get a bit in the IER/IXR register. 
 * @param[in] fd userspace file descriptor
 * @param[in] dir FIFO Direction (HB_MC_FIFO_TO_DEVICE, or HB_MC_FIFO_TO_HOST)
 * @param[out] packet Manycore packet to write
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * */
uint32_t hb_mc_fifo_get_ixr_bit(uint8_t fd, hb_mc_direction_t dir, uint32_t reg, uint32_t bit){
	uint64_t addr = hb_mc_mmio_fifo_get_reg_addr(dir, reg);
	return (hb_mc_read32(fd, addr) & (1<<bit)) != 0;
}

/*!
 * gets the vacancy of a AXI MM FIFO.
 * @param[in] fd userspace file descriptor
 * @param[in] dir FIFO Interface Direction (HB_MC_MMIO_FIFO_TO_HOST or
 * HB_MC_MMIO_FIFO_TO_DEVICE)
 * @param[out] vacancy_p will be set to the vacancy of the fifo
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure
 * */
int hb_mc_fifo_get_vacancy (uint8_t fd, hb_mc_fifo_tx_t type, uint32_t *vacancy_p) {
	uint32_t vacancy_addr;

	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	vacancy_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET);

	*vacancy_p = hb_mc_read16(fd, vacancy_addr);
	return HB_MC_SUCCESS;
}

/*
 * Writes 128B to one of the interface FIFOs
 * @param[in] fd userspace file descriptor
 * @param[in] type transmit packet type (HB_MC_FIFO_TX_REQ for request
 *    packets, HB_MC_FIFO_TX_RSP for response packets)
 * @param[in] packet Manycore packet to write
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * */
int hb_mc_fifo_transmit (uint8_t fd, hb_mc_fifo_tx_t type, const hb_mc_packet_t *packet) {
	uint32_t vacancy; 
	uint32_t data_addr;
	uint32_t isr_addr;
	uint32_t len_addr;
	hb_mc_direction_t dir;

	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_fifo_transmit(): device not initialized.\n");
		return HB_MC_FAIL;
	}	

	data_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_DATA_OFFSET);
	isr_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_DATA_OFFSET);
	len_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_LENGTH_OFFSET);

	dir = hb_mc_get_tx_direction(type);
	hb_mc_fifo_get_vacancy(fd, type, &vacancy);
	if (vacancy < (sizeof(hb_mc_packet_t)/sizeof(uint32_t))) {
		fprintf(stderr, "hb_mc_fifo_transmit(): not enough space in fifo.\n");
		return HB_MC_FAIL;
	}

	// Write 1 to the Transmit Complete bit to clear it
	hb_mc_fifo_set_ixr_bit(fd, dir, HB_MC_MMIO_FIFO_ISR_OFFSET, HB_MC_MMIO_FIFO_IXR_TC_BIT);

	// Transmit the data
	for (int i = 0; i < (sizeof(hb_mc_packet_t)/sizeof(uint32_t)); i++) {
 		hb_mc_write32(fd, data_addr, packet->words[i]);
	}
	
	// Wait for the Transmit Complete bit to get set, while repeatedly writing the size of the packet
	while(!hb_mc_fifo_get_ixr_bit(fd, dir, HB_MC_MMIO_FIFO_ISR_OFFSET, HB_MC_MMIO_FIFO_IXR_TC_BIT)){
		hb_mc_write16(fd, len_addr, sizeof(hb_mc_packet_t));
	}

	// Write 1 to the Transmit Complete bit to clear it
	hb_mc_fifo_set_ixr_bit(fd, dir, HB_MC_MMIO_FIFO_ISR_OFFSET, HB_MC_MMIO_FIFO_IXR_TC_BIT);

	#ifdef DEBUG
	fprintf(stderr, "Fifo packet trasmitted: src (%d,%d), dst (%d,%d), addr: 0x%x, data: 0x%x.\n", packet->request.x_src, packet->request.y_src, packet->request.x_dst, packet->request.y_dst, packet->request.addr, packet->request.data);
	#endif

	return HB_MC_SUCCESS;
}

/*
 * Reads 128B from one of the interface FIFOs
 * @param[in] fd userspace file descriptor
 * @param[in] type transmit packet type (HB_MC_FIFO_RX_REQ for request
 *    packets, HB_MC_FIFO_RX_RSP for response packets)
 * @param[out] packet Manycore packet to receive
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 * */
int hb_mc_fifo_receive (uint8_t fd, hb_mc_fifo_rx_t type, hb_mc_packet_t *packet) {
	uint32_t length_addr;
	uint32_t data_addr;
	uint32_t occupancy;
	uint16_t length;

	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		return HB_MC_FAIL;
	}

	length_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_RX_LENGTH_OFFSET);
	data_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_RX_DATA_OFFSET);

	do {
		hb_mc_fifo_get_occupancy(fd, type, &occupancy);
	}while (occupancy < 1); 

	length = hb_mc_read16(fd, length_addr);
	if (length != sizeof(hb_mc_packet_t)) { 
		return HB_MC_FAIL;
	}
	
	#ifdef DEBUG
	fprintf(stderr, "hb_mc_fifo_receive(): read the receive length register @ %u to be %u\n", length_addr, length);
	#endif

	for (int i = 0; i < sizeof(hb_mc_packet_t)/sizeof(uint32_t); i++) {
		packet->words[i] = hb_mc_read32(fd, data_addr);
	}

	#ifdef DEBUG
	fprintf(stderr, "Fifo packet received: src (%d,%d), dst (%d,%d), addr: 0x%x, data: 0x%x.\n", packet->request.x_src, packet->request.y_src, packet->request.x_dst, packet->request.y_dst, packet->request.addr, packet->request.data);
	#endif

	return HB_MC_SUCCESS;
}

/*
 * @param fd userspace file descriptor
 * @return number of host credits on success and HB_MC_FAIL on failure.
 */
int hb_mc_get_host_credits (uint8_t fd) {
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_host_credits(): device not initialized.\n");
		return HB_MC_FAIL;
	}		
	return hb_mc_read32(fd, hb_mc_mmio_credits_get_reg_addr(HB_MC_MMIO_CREDITS_HOST_OFFSET));
}

/*!
 * Checks that all host requests have been completed.
 * @param fd userspace file descriptor
 * @return HB_MC_SUCCESS if all requests have been completed and HB_MC_FAIL otherwise.
 * */
int hb_mc_all_host_req_complete(uint8_t fd) {
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_host_req_complete(): device not initialized.\n");
		return HB_MC_FAIL;
	}		
	if (hb_mc_get_host_credits(fd) == HB_MC_MMIO_MAX_CREDITS)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}		

/*!
 * Gets a word from the Manycore ROM
 * @param[in] fd userspace file descriptor
 * @param[in] id a configuration register ID
 * @param[out] cfg configuration value pointer to store data in
 * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
 */
int hb_mc_get_config(uint8_t fd, hb_mc_config_id_t id, uint32_t *cfg){
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_config(): device not initialized.\n");
		return HB_MC_FAIL;
	}	
	if ((id < 0) || (id > HB_MC_CONFIG_MAX)) {
		fprintf(stderr, "hb_mc_get_config(): invalid configuration ID.\n");
		return HB_MC_FAIL;
	}
	uint32_t rom_addr_byte = HB_MC_MMIO_ROM_BASE + (id << 2);
	*cfg = hb_mc_read32(fd, rom_addr_byte);
	return HB_MC_SUCCESS;
}

/*!
 * Gets the receive vacancy of the FIFO where the Host is the master.
 * @param fd userspace file descriptor
 * @return the receive vacancy of the FIFO on success and HB_MC_FAIL on failure.
 */
int hb_mc_get_recv_vacancy (uint8_t fd) {
	uint32_t credit_addr;
	if (hb_mc_fifo_check(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "hb_mc_get_recv_vacancy(): device not initialized.\n");
		return HB_MC_FAIL;
	}
	credit_addr = hb_mc_mmio_credits_get_reg_addr(HB_MC_MMIO_CREDITS_FIFO_HOST_VACANCY_OFFSET);
	return hb_mc_read32(fd, credit_addr);
}

/*!
 * @param fd user-level file descriptor.
 * @return the number of columns in the Manycore.
 * */
uint8_t hb_mc_get_manycore_dimension_x () {
	return hb_mc_manycore_dim_x;
} 

/*!
 * @param fd user-level file descriptor.
 * @return the number of rows in the Manycore.
 * */
uint8_t hb_mc_get_manycore_dimension_y () {
	return hb_mc_manycore_dim_y;
}

/*!
 * @param fd user-level file descriptor.
 * @return the host interface X coordinate in Manycore.
 * */
uint8_t hb_mc_get_host_intf_coord_x () {
	return hb_mc_host_intf_coord_x;
}

/*!
 * @param fd user-level file descriptor.
 * @return the host interface Y coordinate in Manycore.
 * */
uint8_t hb_mc_get_host_intf_coord_y () {
	return hb_mc_host_intf_coord_y;
}




