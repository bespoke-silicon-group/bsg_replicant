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

/* This is a skeleton for running a manycore program. It runs the program on Tile (0, 0). */

int main () {
	
	uint8_t fd;
	char *path_to_manycore_program = ""; /* Put path to the riscv file here */
	if (!hb_mc_init_host(path_to_manycore_program, &fd)) {
		printf("failed to initialize host.\n");
		return 0;
	}

	uint8_t x = 0, y = 0;

        hb_mc_freeze(fd, 0, 0);
	
	hb_mc_load_binary(fd, path_to_manycore_program, &x, &y, 1);

  	hb_mc_unfreeze(fd, 0, 0);

	/* Assuming that the manycore program sends a packet to the host, this code attempts to read the packet */
	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */	
	uint32_t *receive_packet = hb_mc_read_fifo(fd, 1, NULL);
	printf("Receive packet: ");
	hb_mc_print_hex((uint8_t *) receive_packet);

	return 0;
}
