#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <bsg_manycore_driver.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_print.h>

int main () {
	
	printf("Running the Manycore-Cache-Loopback test on a 4x4.\n\n");

	uint8_t fd;
	if (!hb_mc_init_host(&fd)) {
		printf("failed to initialize host.\n");
		return 0;
	}


	// check the manycore dimension
//	if (!check_dim(ocl_base)) {
//		printf("Manycore dimensions in FPGA are not as expected.\n");
//		return 0;
//	}

	uint8_t x = 0, y = 1;

        hb_mc_freeze(fd, 0, 1);

	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);
	printf("file to be loaded is %s\n", getenv("MAIN_LOOPBACK"));
	hb_mc_load_binary(fd, getenv("MAIN_LOOPBACK"), &x, &y, 1);

  	hb_mc_unfreeze(fd, 0,1);

	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */	
	uint32_t *receive_packet = hb_mc_read_fifo(fd, 1, NULL);
	printf("Receive packet: ");
	hb_mc_print_hex((uint8_t *) receive_packet);

	return 0;

	//if (!all_req_complete())
	//	printf("binary test: warning - there are outstanding host requests.\n");
}
