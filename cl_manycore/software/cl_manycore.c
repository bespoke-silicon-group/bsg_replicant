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

#include "cosim_rom_test.h"
#include "cosim_read_write_test.h"
#include "cosim_load_vector_test.h"
#include "cosim_loopback_test.h"

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
#define VLED_REG_ADDR   UINT64_C(0x504)

/* SV_TEST macro should be set if SW/HW co-simulation is enabled */
#ifndef SV_TEST
const struct logger *logger = &logger_stdout;
/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot. 
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

int check_afi_ready(int slot_id);

void usage(char* program_name) {
    printf("usage: %s [--slot <slot-id>][<poke-value>]\n", program_name);
}

#endif

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
    // cosim_read_write_test();
    // cosim_load_vector_test();
    // cosim_loopback_test();
    cosim_rom_test();
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



#ifdef SV_TEST
/*This function is used transfer string buffer from SV to C.
  This function currently returns 0 but can be used to update a buffer on the 'C' side.*/
int send_rdbuf_to_c(char* rd_buf)
{
    return 0;
}

#endif
