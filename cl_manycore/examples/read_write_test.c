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

int main () {
	
	printf("Running the Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd;
	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		printf("failed to initialize host.\n");
		return 0;
	}

	
	/* store data in tile */
	uint32_t data = 0xABCD;
	printf("write packet data: 0x%x\n", data);
	int write = hb_mc_copy_to_epa(fd, 0, 1, DMEM_BASE >> 2, &data, 1);

	if (write != HB_MC_SUCCESS) {
		printf("writing data to tile (0, 1)'s DMEM failed.\n");
		return 0;
	}

	/* read back data */
	hb_mc_response_packet_t buf[1];
	int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 1, DMEM_BASE >> 2, 1); 
	if (read == HB_MC_SUCCESS) {
		printf("read packet data: 0x%x\n", hb_mc_response_packet_get_data(&buf[0]));
	}
	
	else {
		printf("read from tile failed.\n");
	}

	return 0;
}

