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


	
#define CROSSBAR_M1           UINT64_C(0x00001000)

#define CFG_REG           UINT64_C(0x1000 + 0x0)
#define CNTL_START        UINT64_C(0x1000 + 0x8)  			// WR
#define CNTL_RESET        UINT64_C(0x1000 + 0xc)  			// W

#define WR_ADDR_LOW       UINT64_C(0x1000 + 0x20)
#define WR_ADDR_HIGH      UINT64_C(0x1000 + 0x24)
#define WR_HEAD           UINT64_C(0x1000 + 0x28)
#define WR_LEN            UINT64_C(0x1000 + 0x2c)
#define WR_OFFSET         UINT64_C(0x1000 + 0x30)
#define RD_ADDR_LOW       UINT64_C(0x1000 + 0x40)
#define RD_ADDR_HIGH      UINT64_C(0x1000 + 0x44)
#define RD_DATA           UINT64_C(0x1000 + 0x48)
#define RD_LEN            UINT65_C(0x1000 + 0x4c)

#define WR_DST_SEL        0x1000 + 0xe0

#define WR_SEL            0x1000 + 1
#define ED_SEL            0x1000 + 2

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

void usage(char* program_name) {
    printf("usage: %s [--slot <slot-id>][<poke-value>]\n", program_name);
}
uint32_t byte_swap(uint32_t value);
#endif

int head_tail_dma(uint32_t value, int slot_id, int pf_id, int bar_id);
void check_mem(int num_pages);
void enable_datagen();
void disable_datagen();
int pop_data (int pop_size); 

uint32_t byte_swap(uint32_t value) {
    uint32_t swapped_value = 0;
    int b;
    for (b = 0; b < 4; b++) {
        swapped_value |= ((value >> (b * 8)) & 0xff) << (8 * (3-b));
    }
    return swapped_value;
}

/* global variables */
char *host_memory_buffer; 
size_t buffer_size = 4*1024*1024;
uint32_t head = 0; 
uint8_t user_buf[128];

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

    printf("===== Head/Tail DMA =====\n");
    
	/* allocate DMA buffer/and config the CL */
	rc = head_tail_dma(value, slot_id, FPGA_APP_PF, APP_PF_BAR4);
    fail_on(rc, out, "Head/Tail DMA failed");
	
	sv_pause(10);				   
	check_mem(5); /* check buffer data against pattern */
	  
	printf("Now start popping off data.\n");
	int num_pops = 0, pop_size = 0, pop_fail = 0;
	while (num_pops <= 20) {
		pop_size = (num_pops % 2) ? 64 : 128;
		pop_fail = pop_data(pop_size);
		if(!pop_fail)
		if (num_pops == 4) 
			disable_datagen();
		else if (num_pops == 13)
    		enable_datagen();	
		num_pops++;
		sv_pause(1);
	}

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

void print_mem () {
	// print out memory 
	sleep(1);  // ensure that axi packets are written into host memory
	for (int i = 0; i < buffer_size + 64; i++) {
		printf("%02X", 0xff & host_memory_buffer[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
		else
			printf(" ");
	} 
}

/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int head_tail_dma(uint32_t value, int slot_id, int pf_id, int bar_id) {
    int rc;
	// uint32_t pcim_data = 0x6c93af50;
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
	size_t align = 64;
	host_memory_buffer = (char *) aligned_alloc(align, buffer_size + 64);
	memset(host_memory_buffer, 0, buffer_size + 64);
	sv_map_host_memory(host_memory_buffer);


	/* get low and high parts of host memory address */
	unsigned long addr_long = (unsigned long) host_memory_buffer;
	uint32_t low32 = (uint32_t) (addr_long & 0x00000000ffffffff);
	uint32_t high32 = (uint32_t) ((addr_long & 0xffffffff00000000) >> 32);
	printf("Host memory address: %lx\n", addr_long);
	
	// Setup the DMA
	rc = fpga_pci_poke(pci_bar_handle, CFG_REG, 0x10); 
    fail_on(rc, out, "Couldn't write to CFG_REG.");
	rc = fpga_pci_poke(pci_bar_handle, WR_ADDR_LOW, low32); // write address low
    fail_on(rc, out, "Couldn't write to WR_ADDR_LOW.");
	rc = fpga_pci_poke(pci_bar_handle, WR_ADDR_HIGH, high32); // write address high
    fail_on(rc, out, "Couldn't write to WR_ADDR_HIGH.");
   	rc = fpga_pci_poke(pci_bar_handle, WR_HEAD, head);     
	fail_on(rc, out, "Couldn't write to WR_HEAD.");
   	rc = fpga_pci_poke(pci_bar_handle, WR_LEN, 0x0); // write 64 bytes, 512bits
    fail_on(rc, out, "Couldn't write to WR_LEN.");
    rc = fpga_pci_poke(pci_bar_handle, WR_OFFSET, (uint32_t) (buffer_size)); 
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

   	rc = fpga_pci_poke(pci_bar_handle, CNTL_START, WR_SEL);
    fail_on(rc, out, "Couldn't write to CNTL_START.");

	int seconds = 0;


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


void check_mem (int num_pages) {
	
	bool pass = true;
	uint32_t counter = 0x0;

	int end = ((num_pages * 4096) >  (buffer_size - 64)) ? (buffer_size - 64) : (num_pages * 4096);
	printf("end: %d\n", end);
	for (int fsb = 2*16; fsb < (end/10); fsb += 16) {
		for (int i = 0; i < 8; i++) { // data bytes
			uint8_t byte = host_memory_buffer[fsb + i];
			uint8_t id = (byte & 0xE0) >> 5; 
			uint8_t data = byte & (0x1F);			
			if (id != i || data != counter) {
				printf("check_mem(): mismatch @ fsb #%d\n", fsb);
				pass = false;
				goto test_finish;
			} 
		}		
		for (int i = 8; i < 10; i++) {
			// check header
		}
		counter = (counter + 1) % 32;
	}	
	test_finish:
	if (pass) 
		printf("Pass! Memory is correct.\n");
	else
		printf("Fail!\n");
}

int pop_data (int pop_size) {
    
    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
	if (fpga_pci_init())
		printf("Unable to initialize fpga_pci library.\n");


	/* check if there is data to read */
	uint32_t *tail = (uint32_t *) (host_memory_buffer + buffer_size);
	printf("pop_data(): tail is at 0x%x\n", *tail);
	int old_head = head; // for debugging messages 
	bool can_read;
	uint32_t num_dw;
	if (*tail >= head)
		num_dw = *tail - head;
	else
		num_dw = *tail - head + buffer_size;
	can_read = num_dw >= pop_size;
	if (!can_read) {
		printf("pop_data(): can't read %d bytes because (Head, Tail) = (%d, %d);\n only %d bytes available. Will try this again.\n",  pop_size, head, *tail, num_dw); 
		return 1;
	}
	/* there is enough unread data; first, read data that lies before the end of system memory buffer */
	uint32_t before_dw = (buffer_size - head >= pop_size) ? pop_size : buffer_size - head; 
	for (int i = 0; i < before_dw; i++) 
		user_buf[i] = host_memory_buffer[head + i];		
	head = (head + before_dw) % buffer_size;
	/* read data that wraps over the end of system memory buffer */
	uint32_t after_dw = pop_size - before_dw;
	if (after_dw > 0) { /* if there is still data to read */
		/* read this data into a user buffer */
		for (int i = 0; i < after_dw; i++) 
			user_buf[before_dw + i] = host_memory_buffer[i];
		head = (head + after_dw) % buffer_size; /* update head */	
	}

   	if (fpga_pci_poke(PCI_BAR_HANDLE_INIT, WR_HEAD, head)) /* update head register on device */
		printf("Couldn't write to head register.\n"); 

	/* print what has been read */
	printf("User program has popped data at [%u, %u)", old_head, old_head + before_dw);
	if (after_dw)
		printf(" and [0, %u):\n", after_dw);
	else
		printf(":\n");
	for (int i = 0; i < pop_size; i++) {
		printf("0x%02X", (uint32_t) user_buf[i]);
		if ((i + 1) % 16 == 0) 
			printf("\n");
		else
			printf(" ");
	}	
	printf("\n");
	return 0;
}

void disable_datagen () {
    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
	printf("disable_datagen()\n");
	if (fpga_pci_init())
		printf("Unable to initialize fpga_pci library.\n");
	
	int rc = fpga_pci_poke(PCI_BAR_HANDLE_INIT, CFG_REG, 0); 

	if (rc)
		printf("disable_datagen(): error writing to CFG_REG.\n");
	else
		printf("disable_datagen(): successfully disabled datagen.\n");
}

void enable_datagen () {
    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
	printf("enable_datagen()\n");
	if (fpga_pci_init())
		printf("Unable to initialize fpga_pci library.\n");


	int rc = fpga_pci_poke(PCI_BAR_HANDLE_INIT, CFG_REG, 0x10); 
   	rc = fpga_pci_poke(PCI_BAR_HANDLE_INIT, CNTL_START, WR_SEL);
	
	if (rc)
		printf("enable_datagen(): error writing to CFG_REG.\n");
	else
		printf("enable_datagen(): successfully enabled datagen.\n");
}

#ifdef SV_TEST
/*This function is used transfer string buffer from SV to C.
  This function currently returns 0 but can be used to update a buffer on the 'C' side.*/
int send_rdbuf_to_c(char* rd_buf)
{
   return 0;
}

#endif
