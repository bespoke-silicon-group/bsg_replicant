#include <stdint.h>

#ifdef SV_TEST
   #include "fpga_pci_sv.h"
#else
   #include <fpga_pci.h>
   #include <fpga_mgmt.h>
   #include <utils/lcd.h>
#endif

#include <utils/sh_dpi_tasks.h>


uint32_t WRITE_COUNT_REG = 0x80000014;
uint32_t WRITE_REG = 0x80000010;
uint32_t AVAIL_REG = 0x80000024;
uint32_t READ_REG = 0x80000020; 

uint16_t SOURCE = 0;
int AVX_SIZE = 32;
int FSB_SIZE = 16;
int AXI_SIZE = 4;

void _store_epu32(pci_bar_handle_t bar, uint32_t *payload) {
	for (int i = 0; i < AVX_SIZE / AXI_SIZE; i++) {
		int rc = fpga_pci_poke(bar, WRITE_REG, payload[i]); 
		if (rc){ 
			printf("Unable to write to write register!\n");
			return; 
		}
		if ((i + 1) % (FSB_SIZE / AXI_SIZE) == 0) { /* finished writing an FSB packet */
			rc = fpga_pci_poke(bar, WRITE_COUNT_REG, FSB_SIZE); 
			if (rc){ 
				printf("Unable to write to write register!\n");
				return; 
			}
		}
	}
}

void _load_epu32(pci_bar_handle_t bar, uint32_t *payload) {
	for (int i = 0; i < AVX_SIZE / AXI_SIZE; i++) {
		int rc;
		int avail; 
		if (i % (FSB_SIZE / AXI_SIZE) == 0) { /* start of a new FSB packet */
			int avail; 
			rc = fpga_pci_peek(bar, AVAIL_REG, &avail); 
			if (rc) {
				printf("Unable to read from avail register.\n");
				return;
			}
			else if (avail < FSB_SIZE){ 
				printf("Not enough data to read!\n");
				return; 
			}
		}
		rc = fpga_pci_peek(bar, READ_REG, &payload[i]); 
		if (rc) {
			printf("Unable to read from read register.\n");
			return; 
		}	
	}
}

void store_epu32(uint32_t *data, uint16_t dest) {
	/*
	 * Sequence of DW writes: 
	 * 	1) payload 2 
	 * 	2) payload 1 
	 * 	3) header 
	 * 	4) unused
	 * 	5) payload 4
	 * 	6) payload 3
	 * 	7) header
	 * 	8) unused
	 * 	. . .		
	 */	

	printf("Args: data pointer: %p, dest ID: %d", data, dest);
	printf("Not implemented yet!\n");
}	

void store_epu64(uint64_t *data, uint16_t dest) {
	printf("Args: data pointer: %p, dest ID: %d", data, dest);
	printf("Not implemented yet!\n");
}


void store_epu16(uint16_t * data, uint16_t dest) {
	printf("Args: data pointer: %p, dest ID: %d", data, dest);
	printf("Not implemented yet!\n");
}


