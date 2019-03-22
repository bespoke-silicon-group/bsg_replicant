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
#include <bsg_manycore_errno.h>

int main () {

	char *manycore_program = "";	
	uint8_t fd;
	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		printf("failed to initialize host.\n");
		return 0;
	}

	uint8_t x = 0, y = 1;

        hb_mc_freeze(fd, 0, 1);

	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);
	printf("file to be loaded is %s\n", manycore_program);
	hb_mc_load_binary(fd, manycore_program, &x, &y, 1);

  	hb_mc_unfreeze(fd, 0,1);

	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */	
	uint32_t *receive_packet = hb_mc_read_fifo(fd, 1, NULL);
	printf("Receive packet: ");
	hb_mc_print_hex((uint8_t *) receive_packet);

	return 0;

}
