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
	
	printf("Running the Manycore-Cache-Loopback test on a 4x4.\n\n");

	uint8_t fd;
	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
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
	
	request_packet_t manycore_finish;
	hb_mc_read_fifo(fd, 1, &manycore_finish);	

	uint32_t addr = request_packet_get_addr(&manycore_finish);
	uint32_t data = request_packet_get_data(&manycore_finish);
	uint32_t op_ex = request_packet_get_op_ex(&manycore_finish);
	uint32_t x_src = request_packet_get_x_src(&manycore_finish);
	uint32_t y_src = request_packet_get_y_src(&manycore_finish);
	uint32_t x_dst = request_packet_get_x_dst(&manycore_finish);
	uint32_t y_dst = request_packet_get_y_dst(&manycore_finish);
	uint32_t op = request_packet_get_op(&manycore_finish);
	printf("Manycore finish packet received at Address 0x%x at coordinates (0x%x, 0x%x) from (0x%x, 0x%x). Operation: 0x%x, Data: 0x%x\n", addr, x_dst, y_dst, x_src, y_src, op, data & op_ex);


	return 0;

	//if (!all_req_complete())
	//	printf("binary test: warning - there are outstanding host requests.\n");
}
