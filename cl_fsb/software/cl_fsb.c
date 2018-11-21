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

#ifdef SV_TEST
   #include "fpga_pci_sv.h"
#else
   #include <fpga_pci.h>
   #include <fpga_mgmt.h>
   #include <utils/lcd.h>
#endif

#include <utils/sh_dpi_tasks.h>

#include "axiintrin.h"

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
    rc = peek_poke_example(value, slot_id, FPGA_APP_PF, APP_PF_BAR0);
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
    
	/* Create 256 bit payload to write */
	uint32_t expected[8];
	for (int i = 0; i < 8; i++) {
		expected[i] = value;
	}
    /* write a value into the mapped address space */
	printf("Writing to device.\n");
	_store_epu32(pci_bar_handle, expected); 
	printf("Wrote to device.\n");
	/* read back value */
	uint32_t read[8];
	printf("Reading from device.\n");
	_load_epu32(pci_bar_handle, read);
	printf("Read from device.\n");
	/* check if expected = read */
	printf("Checking correctness.\n");
	bool pass = true;
	for (int i = 0; i < 8; i++) {
		int packet_type = i % (FSB_SIZE / AXI_SIZE);
		switch (packet_type) {
			case 0:
			case 1: { // data DW 
				if (expected[i] != read[i]) {
					printf("Mismatch at %i. Expected: %x. Read: %x.\n", i, expected[i], read[i]);
					pass = false;
				}
				break;
			}
			case 2: { // header DW
				int temp = 	((expected[i] & 0x0000000F)<<12) + ((expected[i] >> 12) & 0x0000000F) + (expected[i] & 0x00000FF0);
				if (read[i] != temp) {
					printf("Mismatch at %i. Expected: %x. Read: %x.\n", i, expected[i], read[i]);
					pass = false;

				}
				break;
			}
			default: {}
		}
	}
    
    if(pass)
        printf("TEST PASSED\n");
    else
        printf("TEST FAILED");
    
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
