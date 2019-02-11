#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "deploy.h"
#include "../device.h"
#include "fifo.h"
#include "loader/spmd_loader.h"


int main () {
	
	printf("Running the Manycore-Cache-Loopback test on a 4x4.\n\n");

	/* Setup host */
	struct Host *host = (struct Host *) malloc(sizeof(struct Host));	 
	deploy_init_host(host, 0, 0); // DMA arguments unused	

	/* mmap the OCL BAR */
	char *ocl_base = deploy_mmap_ocl();
	if (ocl_base == 0) {
		printf("Error when mmap'ing OCL Bar.\n");
		return 0;
	}


	/*---------------------------------------------------------------------------*/
	// icache init
	/*---------------------------------------------------------------------------*/
	printf("Initializing the icache of tile (0, 0) ...\n");
	uint8_t **packets_icache = init_icache();
	int num_icache_packets = NUM_ICACHE;	
	bool pass_icache = true;
	
	for (int i = 0; i < num_icache_packets; i++) {
		if (!deploy_write_fifo(0, (uint32_t *) packets_icache[i])) {
			pass_icache = false;
			break;
		}
	}
	if (pass_icache) 
		printf("icache init finished.\n");
	else
		printf("icache init failed.\n");
  /*---------------------------------------------------------------------------*/
  // vcache init
  /*---------------------------------------------------------------------------*/
	printf("Initializing the vcaches\n");
	uint32_t num_tags = NUM_VCACHE_ENTRY * VCACHE_WAYS;
	uint32_t num_vcache_packets = NUM_VCACHE * num_tags; 
	uint8_t **packets_vcache = init_vcache();
	bool pass_vcache = true;
	
	for (int i = 0; i < num_vcache_packets; i++) {
		if (!deploy_write_fifo(0, (uint32_t *) packets_vcache[i])) {
			pass_vcache = false;
			break;
		}
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
	bool pass_dram = true;
	
	for (int i = 0; i < num_text_pkts; i++) {
		if (!deploy_write_fifo(0, (uint32_t *) text_pkts[i])) {
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
	bool pass_dmem = true;
	
	for (int i = 0; i < num_data_pkts; i++) {
		if (!deploy_write_fifo(0, (uint32_t *) data_pkts[i])) {
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
	if (!deploy_write_fifo(0, (uint32_t *) unfreeze_pkts[0])) {
		pass_unfreeze = false;
	}
	if (pass_unfreeze)
		printf("unfreeze finished.\n");
	else
		printf("unfreeze failed.\n");	
	/*---------------------------------------------------------------------------*/
	// check receive packet 
	/*---------------------------------------------------------------------------*/
	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */	
	uint32_t *receive_packet = deploy_read_fifo(0, NULL);
	printf("Receive packet: ");
	print_hex((uint8_t *) receive_packet);

	return 0;
}
