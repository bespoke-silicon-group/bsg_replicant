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
#include <unistd.h>
#include <stdlib.h>
#include "cl_utils.h"

#ifdef SV_TEST
   #include "fpga_pci_sv.h"
#else
   #include <fpga_pci.h>
   #include <fpga_mgmt.h>
   #include <utils/lcd.h>
#endif

#include <utils/sh_dpi_tasks.h>

#include "axiintrin.h"

#define CFG_REG           UINT64_C(0x0)
#define CNTL_START        UINT64_C(0x8)  			// WR
#define CNTL_RESET        UINT64_C(0xc)  			// W

#define WR_ADDR_LOW       UINT64_C(0x20)
#define WR_ADDR_HIGH      UINT64_C(0x24)
#define WR_DATA           UINT64_C(0x28)
#define WR_LEN            UINT64_C(0x2c)
#define WR_OFFSET         UINT64_C(0x30)

#define RD_ADDR_LOW       UINT64_C(0x40)
#define RD_ADDR_HIGH      UINT64_C(0x44)
#define RD_DATA           UINT64_C(0x48)
#define RD_LEN            UINT65_C(0x4c)

#define WR_DST_SEL        0xe0

#define WR_SEL            1
#define ED_SEL            2

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

char *host_memory_buffer; 

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

    printf("===== Starting with peek_poke_example =====\n");
    rc = peek_poke_example(value, slot_id, FPGA_APP_PF, APP_PF_BAR4);
    fail_on(rc, out, "peek-poke example failed");

    printf("Developers are encouraged to modify the Virtual DIP Switch by calling the linux shell command to demonstrate how AWS FPGA Virtual DIP switches can be used to change a CustomLogic functionality:\n");
    printf("$ fpga-set-virtual-dip-switch -S (slot-id) -D (16 digit setting)\n\n");
    printf("In this example, setting a virtual DIP switch to zero clears the corresponding LED, even if the peek-poke example would set it to 1.\nFor instance:\n");

    printf(
        "# sudo fpga-set-virtual-dip-switch -S 0 -D 1111111111111111\n"
        "# sudo fpga-get-virtual-led  -S 0\n"
        "FPGA slot id 0 have the following Virtual LED:\n"
        "1010-1101-1101-1110\n"
        "# sudo fpga-set-virtual-dip-switch -S 0 -D 0000000000000000\n"
        "# sudo fpga-get-virtual-led  -S 0\n"
        "FPGA slot id 0 have the following Virtual LED:\n"
        "0000-0000-0000-0000\n"
    );

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
	uint64_t pcim_addr = 0x0000000012340000;
	uint32_t pcim_data = 0x6c93af50;
	uint32_t read_data;


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
	
	/* Allocate host memory and tell cosimulation functions to use this memory. */
	host_memory_buffer = (char *) aligned_alloc(64, 4 * 320);
	sv_map_host_memory(host_memory_buffer);


	/* get low and high parts of host memory address */
	unsigned long addr_long = (unsigned long) host_memory_buffer;
	uint32_t low = (uint32_t) (addr_long & 0x00000000ffffffff);
	uint32_t high = (uint32_t) ((addr_long & 0xffffffff00000000) >> 32);
	printf("Host memory address: %lx\n", addr_long);
	
	// Setup the DMA
	rc = fpga_pci_poke(pci_bar_handle, CFG_REG, 0x8); 
    	fail_on(rc, out, "Couldn't write to CFG_REG.");
	rc = fpga_pci_poke(pci_bar_handle, WR_ADDR_LOW, low32); // write address low
    	fail_on(rc, out, "Couldn't write to WR_ADDR_LOW.");
	rc = fpga_pci_poke(pci_bar_handle, WR_ADDR_HIGH, high32); // write address high
    	fail_on(rc, out, "Couldn't write to WR_ADDR_HIGH.");
   	rc = fpga_pci_poke(pci_bar_handle, WR_DATA, pcim_data); // write data, not used
    	fail_on(rc, out, "Couldn't write to WR_DATA.");
   	rc = fpga_pci_poke(pci_bar_handle, WR_LEN, 0x0); // write 64 bytes, 512bits
    	fail_on(rc, out, "Couldn't write to WR_LEN.");
    	rc = fpga_pci_poke(pci_bar_handle, WR_OFFSET, (uint32_t) (0x140-1)); // 320 bytes, 32 fsb pkts
   	fail_on(rc, out, "Couldn't write to WR_OFFSET.");

	// start write
   	rc = fpga_pci_poke(pci_bar_handle, CNTL_START, WR_SEL);
    	fail_on(rc, out, "Couldn't write to CNTL_START.");

	// wait for writes to complete
	int timeout_count = 0;
    	do {
		sleep(1);
        	printf("Waiting for 1st write activity to complete: %d ms.\n", timeout_count);
		rc = fpga_pci_peek(pci_bar_handle, CNTL_START, &read_data);
		fail_on(rc, out, "Couldn't read from CNTL_START"); 
        	timeout_count++;
    	} while (((read_data & 1) != 0) && timeout_count < 10);

     // check if DMA is complete
  	if (((read_data & 1) != 0) && (timeout_count == 10))
     		printf("Timeout waiting for 1st writes to complete.\n");
   	else
     		printf("PASS~~~ axi4 1st writes complete.\n");
     	if (read_data & 8) // check bit 3
     		printf("axi4 1st writes RESP ERROR.\n");

  
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


void print_mem () {
	// print out memory 
	sleep(1);  // ensure that axi packets are written into host memory
	printf("Memory after DMA: \n");
	for (int i = 0; i < 320*4; i++) {
		printf("%02X", 0xff & host_memory_buffer[i]);
		if (i % 10 == 0 && i > 0)
			printf("\n");
		else
			printf(" ");
} 



}

#ifdef SV_TEST
/*This function is used transfer string buffer from SV to C.
  This function currently returns 0 but can be used to update a buffer on the 'C' side.*/
int send_rdbuf_to_c(char* rd_buf)
{
   return 0;
}

#endif
