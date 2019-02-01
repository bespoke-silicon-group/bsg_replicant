// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "deploy/loader/spmd_loader.h"
#include "deploy/fifo.h"

#ifdef SV_TEST
   #include "fpga_pci_sv.h"
#else
   #include <fpga_pci.h>
   #include <fpga_mgmt.h>
   #include <utils/lcd.h>
#endif

#include <utils/sh_dpi_tasks.h>


/* 
 * Register offsets determined by the CL HDL. These addresses should match the
 * addresses in the verilog headers 
 */
#define HELLO_WORLD_REG_ADDR UINT64_C(0x500)
#define VLED_REG_ADDR	UINT64_C(0x504)

/* SV_TEST macro should be set if SW/HW co-simulation is enabled */
#ifndef SV_TEST
const struct logger *logger = &logger_stdout;
/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot. 
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

/*
 * check if the corresponding AFI for hello_world is loaded
 */
int check_afi_ready(int slot_id);

/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int peek_poke_example(uint32_t value, int slot_id, int pf_id, int bar_id);

void usage(char* program_name) {
    printf("usage: %s [--slot <slot-id>][<poke-value>]\n", program_name);
}

uint32_t byte_swap(uint32_t value);
#endif

uint32_t byte_swap(uint32_t value) {
    uint32_t swapped_value = 0;
    int b;
    for (b = 0; b < 4; b++) {
        swapped_value |= ((value >> (b * 8)) & 0xff) << (8 * (3-b));
    }
    return swapped_value;
}


uint32_t *read_packet (uint8_t n) {
    int rc;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
   	uint32_t occupancy, receive_length;
	uint32_t *receive_packet = (uint32_t *) (4, sizeof(uint32_t));	

	/* wait until read fifo has packets */
	do {
		printf("read(): checking occupancy of read fifo.\n");
		rc = fpga_pci_peek(pci_bar_handle, fifo[0][FIFO_OCCUPANCY], &occupancy);
		if (rc) {
			printf("Unable to peek!\n");
		}
	} while (occupancy < 1);
	
	/* read back the packet */
	rc = fpga_pci_peek(pci_bar_handle, fifo[0][FIFO_RECEIVE_LENGTH], &receive_length);
	if (receive_length == 16) { /* read back only if receive length is 16B */
		for (int i = 0; i < 4; i++) {
			rc = fpga_pci_peek(pci_bar_handle, fifo[0][FIFO_READ], &receive_packet[i]);
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
/*------------------------------------------------------------------------------*/
// write to fifo
/*------------------------------------------------------------------------------*/
uint8_t *ocl_base = 0;
bool write_packet(uint8_t n, uint8_t *packet) {
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


//"/home/ahari/bsg_manycore/software/spmd/bsg_dram_loopback_cache/main.riscv";
/* 
 * cosim_wrapper.sv calls test_main (not main). Use SV_TEST to switch between
 * the two use-cases. 
 */
#ifdef SV_TEST
//For cadence and questa simulators the main has to return some value
#ifdef INT_MAIN
   int test_main(uint32_t *exit_code) {
#else 
   void test_main(uint32_t *exit_code) {
#endif 
#else 
    int main(int argc, char **argv) {
#endif
    //The statements within SCOPE ifdef below are needed for HW/SW co-simulation with VCS
    #ifdef SCOPE
      svScope scope;
      scope = svGetScopeFromName("tb");
      svSetScope(scope);
    #endif

    uint32_t value = 0xefbeadde;
    int slot_id = 0;
    int rc;
    
#ifndef SV_TEST
    // Process command line args
    {
        int i;
        int value_set = 0;
        for (i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "--slot")) {
                i++;
                if (i >= argc) {
                    printf("error: missing slot-id\n");
                    usage(argv[0]);
                    return 1;
                }
                sscanf(argv[i], "%d", &slot_id);
            } else if (!value_set) {
                sscanf(argv[i], "%x", &value);
                value_set = 1;
            } else {
                printf("error: Invalid arg: %s", argv[i]);
                usage(argv[0]);
                return 1;
            }
        }
    }
#endif

    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    rc = fpga_pci_init();
    fail_on(rc, out, "Unable to initialize the fpga_pci library");

#ifndef SV_TEST
    rc = check_afi_ready(slot_id);
#endif

    fail_on(rc, out, "AFI not ready");
    
    /* Accessing the CL registers via AppPF BAR0, which maps to sh_cl_ocl_ AXI-Lite bus between AWS FPGA Shell and the CL*/

    printf("===== Running SPMD Loader =====\n");
	/*---------------------------------------------------------------------------*/
	// icache init
	/*---------------------------------------------------------------------------*/
	printf("Initializing the icache of tile (0, 0) ...\n");
	uint8_t **packets_icache = init_icache();
	int num_icache_packets = NUM_ICACHE;	
	bool pass_icache = true;
	
	for (int i = 0; i < num_icache_packets; i++) {
		if (!write_packet(0, packets_icache[i])) {
			pass_icache = false;
			break;
		}
		if (i % 200 == 0)
			printf("wrote icache packet %d\n", i);
	}
	if (pass_icache) 
		printf("icache init finished.\n");
	else
		printf("icache init failed.\n");
  /*---------------------------------------------------------------------------*/
  // vcache init
  /*---------------------------------------------------------------------------*/
	printf("Initializing the vcaches");
	uint32_t num_tags = NUM_VCACHE_ENTRY * VCACHE_WAYS;
	uint32_t num_vcache_packets = NUM_VCACHE * num_tags; 
	uint8_t **packets_vcache = init_vcache();
	bool pass_vcache = true;
	
	for (int i = 0; i < num_vcache_packets; i++) {
		printf("packet: ");
		print_hex(packets_vcache[i]);
		if (!write_packet(0, packets_vcache[i])) {
			pass_vcache = false;
			break;
		}
		if (i % 20 == 0)
			printf("wrote vcache packet %d\n", i);
	}
	if (pass_vcache) 
		printf("vcache init finished.\n");
	else
		printf("vcache init failed.\n");
	/*---------------------------------------------------------------------------*/
	// dram init
	/*---------------------------------------------------------------------------*/
	printf("Initializing dram\n");
	parse_elf(getenv("MAIN_LOOPBACK"));
	printf("Number of instructions: 0x%x\n", num_text_pkts);
	bool pass_dram = true;
	
	for (int i = 0; i < num_text_pkts; i++) {
		if (!write_packet(0, text_pkts[i])) {
			pass_dram = false;
			break;
		}
		printf("wrote dram packet %d\n", i);
	}
	if (pass_dram) 
		printf("dram init finished.\n");
	else
		printf("dram init failed.\n");
	/*---------------------------------------------------------------------------*/
	// dmem init
	/*---------------------------------------------------------------------------*/
	printf("Initializing dmem\n");
	printf("Number of data packets: %x\n", num_data_pkts);
	bool pass_dmem = true;
	
	for (int i = 0; i < num_data_pkts; i++) {
		if (!write_packet(0, data_pkts[i])) {
			pass_dmem = false;
			break;
		}
		printf("wrote dmem packet %d\n", i);
	}
	if (pass_dmem) 
		printf("dmem init finished.\n");
	else
		printf("dmem init failed.\n");
	/*---------------------------------------------------------------------------*/
	// unfreeze tile (0,0) 
	/*---------------------------------------------------------------------------*/
	printf("Unfreezing tile (0,0).\n");
	uint8_t **unfreeze_pkts = unfreeze_tiles();
	bool pass_unfreeze = true;
	if (!write_packet(0, unfreeze_pkts[0])) {
		pass_unfreeze = false;
	}
	if (pass_unfreeze)
		printf("unfreeze finished.\n");
	else
		printf("unfreeze failed.\n");	
	/*---------------------------------------------------------------------------*/
	// check receive packet 
	/*---------------------------------------------------------------------------*/
	printf("Checking receive packet, but first waiting for 100 us.\n");
	sv_pause(100); /* 100 us */	
	uint32_t *receive_packet = read_packet(0);
	printf("Receive packet: ");
	print_hex((uint8_t *) receive_packet);
	
#ifndef SV_TEST
    return rc;
    
out:
    return 1;
#else

out:
   #ifdef INT_MAIN
   *exit_code = 0;
   return 0;
   #else 
   *exit_code = 0;
   #endif
#endif
}

/* As HW simulation test is not run on a AFI, the below function is not valid */
#ifndef SV_TEST

 int check_afi_ready(int slot_id) {
   struct fpga_mgmt_image_info info = {0}; 
   int rc;

   /* get local image description, contains status, vendor id, and device id. */
   rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
   fail_on(rc, out, "Unable to get AFI information from slot %d. Are you running as root?",slot_id);

   /* check to see if the slot is ready */
   if (info.status != FPGA_STATUS_LOADED) {
     rc = 1;
     fail_on(rc, out, "AFI in Slot %d is not in READY state !", slot_id);
   }

   printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
          info.spec.map[FPGA_APP_PF].vendor_id,
          info.spec.map[FPGA_APP_PF].device_id);

   /* confirm that the AFI that we expect is in fact loaded */
   if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
       info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
     printf("AFI does not show expected PCI vendor id and device ID. If the AFI "
            "was just loaded, it might need a rescan. Rescanning now.\n");

     rc = fpga_pci_rescan_slot_app_pfs(slot_id);
     fail_on(rc, out, "Unable to update PF for slot %d",slot_id);
     /* get local image description, contains status, vendor id, and device id. */
     rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
     fail_on(rc, out, "Unable to get AFI information from slot %d",slot_id);

     printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
            info.spec.map[FPGA_APP_PF].vendor_id,
            info.spec.map[FPGA_APP_PF].device_id);

     /* confirm that the AFI that we expect is in fact loaded after rescan */
     if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
         info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
       rc = 1;
       fail_on(rc, out, "The PCI vendor id and device of the loaded AFI are not "
               "the expected values.");
     }
   }
    
   return rc;
 out:
   return 1;
 }

#endif

/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int peek_poke_example(uint32_t value, int slot_id, int pf_id, int bar_id) {
    int rc;
    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */

    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

    
    /* attach to the fpga, with a pci_bar_handle out param
     * To attach to multiple slots or BARs, call this function multiple times,
     * saving the pci_bar_handle to specify which address space to interact with in
     * other API calls.
     * This function accepts the slot_id, physical function, and bar number
     */
#ifndef SV_TEST
    rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle);
#endif
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
    
    /* write a value into the mapped address space */
    uint32_t expected = byte_swap(value);
    printf("Writing 0x%08x to HELLO_WORLD register (0x%016lx)\n", value, HELLO_WORLD_REG_ADDR);
    rc = fpga_pci_poke(pci_bar_handle, HELLO_WORLD_REG_ADDR, value);

    fail_on(rc, out, "Unable to write to the fpga !");

    /* read it back and print it out; you should expect the byte order to be
     * reversed (That's what this CL does) */
    rc = fpga_pci_peek(pci_bar_handle, HELLO_WORLD_REG_ADDR, &value);
    
    // cl_peek(HELLO_WORLD_REG_ADDR, &value);
    // printf("Foo!\n");
    fail_on(rc, out, "Unable to read read from the fpga !");
    printf("=====  Entering peek_poke_example =====\n");
    printf("register: 0x%x\n", value);
    if(value == expected) {
        printf("TEST PASSED\n");
        printf("Resulting value matched expected value 0x%x. It worked?!\n", expected);
    }
    else{
        printf("TEST FAILED");
        printf("Resulting value did not match expected value 0x%x. Something didn't work.\n", expected);
    }
out:
    /* clean up */
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            printf("Failure while detaching from the fpga.\n");
        }
    }

    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
}

#ifdef SV_TEST
/*This function is used transfer string buffer from SV to C.
  This function currently returns 0 but can be used to update a buffer on the 'C' side.*/
int send_rdbuf_to_c(char* rd_buf)
{
   return 0;
}

#endif
