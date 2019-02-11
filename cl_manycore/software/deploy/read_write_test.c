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
#include "primitives/primitives.h"

int main () {
	
	printf("Running the Read/Write test on the Manycore with 4 x 4 dimensions.\n\n");

	/* Setup host */
	struct Host *host = (struct Host *) malloc(sizeof(struct Host));	 
	deploy_init_host(host, 0, 0); // DMA arguments unused	
	/* mmap the OCL BAR */
	char *ocl_base = deploy_mmap_ocl();
	if (ocl_base == 0) {
		printf("Error when mmap'ing OCL Bar.\n");
		return 0;
	}

	// check the manycore dimension
	if (!deploy_check_dim()) {
		printf("Manycore dimensions in FPGA are not as expected.\n");
		return 0;
	}


	/* store data in tile */
	uint32_t data = 0xABCD;
	bool write = hb_xeon_to_epa_copy(0, 0, DMEM_BASE >> 2, &data, 1);

	if (!write) {
		printf("writing data to tile (0, 0)'s DMEM failed.\n");
		return 0;
	}

	/* read back data */
	uint32_t **buf = (uint32_t **) calloc(1, sizeof(uint32_t *));
	bool read = hb_epa_to_xeon_copy (buf, 0, 0, DMEM_BASE >> 2, 1); 
	printf("completed read.\n");
	if (read == 1) {
		printf("read packet: ");
		print_hex((uint8_t *) buf[0]);
	}
	else {
		printf("read from tile failed.\n");
	}

	//if (!all_req_complete())
	//	printf("read/write test: warning - there are outstanding host requests.\n");

	return 0;
}
