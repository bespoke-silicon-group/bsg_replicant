#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <bsg_manycore_driver.h>
#include <bsg_manycore_mem.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_packet.h>

static void print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) {
		printf("%x ", (p[15-i] & 0xFF));
	}
	printf("\n");
}

int main () {
	
	printf("Running the Load Vector Test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd;
	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		printf("failed to initialize host.\n");
		return 0;
	}
	
	uint32_t n = 10; 
	uint32_t data[n];
	srand(0);
	for (int i = 0; i < n; i++) {
		data[i] = rand();
		printf("random number %d: 0x%x\n", i, data[i]);
	}

	/* store data in tile */
	int write = hb_mc_copy_to_epa(fd, 0, 1, DMEM_BASE >> 2, (uint32_t *) &data[0], n);

	if (write != HB_MC_SUCCESS) {
		printf("writing data to tile (0, 1)'s DMEM failed.\n");
		return 0;
	}

	/* read back data */
	request_packet_t buf[n];
	int read = hb_mc_copy_from_epa(fd, (request_packet_t *) &buf[0], 0, 1, DMEM_BASE >> 2, n); 
	
	if (read == HB_MC_SUCCESS) {
		printf("read packets: \n");
		for (int i = 0; i < n; i++)
			print_hex((uint8_t *) &buf[i]);
	}
	
	else {
		printf("read from tile failed.\n");
	}

	return 0;
}

