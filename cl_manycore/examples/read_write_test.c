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
#include <bsg_manycore_print.h>

int main () {
	
	printf("Running the Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");

	uint8_t fd;
	if (!hb_mc_init_host(&fd)) {
		printf("failed to initialize host.\n");
		return 0;
	}

	
	/* store data in tile */
	uint32_t data = 0xABCD;
	bool write = hb_mc_copy_to_epa(fd, 0, 1, DMEM_BASE >> 2, &data, 1);

	if (!write) {
		printf("writing data to tile (0, 1)'s DMEM failed.\n");
		return 0;
	}

	/* read back data */
	uint32_t **buf = (uint32_t **) calloc(1, sizeof(uint32_t *));
	bool read = hb_mc_copy_from_epa(fd, buf, 0, 1, DMEM_BASE >> 2, 1); 
	printf("completed read.\n");
	if (read == 1) {
		printf("read packet: ");
		hb_mc_print_hex((uint8_t *) buf[0]);
	}
	
	else {
		printf("read from tile failed.\n");
	}

	return 0;
}

