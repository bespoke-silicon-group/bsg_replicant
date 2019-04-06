#ifndef COSIM_CUDA_TEST_H
#define COSIM_CUDA_TEST_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "bsg_manycore_driver.h"
#include "bsg_manycore_mem.h"
#include "bsg_manycore_loader.h"
#include "bsg_manycore_errno.h"

void cosim_cuda_test () {
	
	printf("Cosimulation test of hb_mc_init_device().\n\n");
	uint8_t fd; 
	hb_mc_init_host(&fd);
	
	/* run on tile (0, 1) */
	tile_t tiles[1];
	tiles[0].x = 0;
	tiles[0].y = 1;
	tiles[0].origin_x = 0;
	tiles[0].origin_y = 1;
	eva_id_t eva_id = 0;
	uint32_t num_tiles = 1;
	if (hb_mc_init_device(fd, eva_id, getenv("MAIN_LOOPBACK"), &tiles[0], num_tiles) != HB_MC_SUCCESS) {
		printf("could not initialize device.\n");
		return;
	}  

	/* make sure memory manager was initialized properly */
	uint32_t start, size;
	_hb_mc_get_mem_manager_info(eva_id, &start, &size); 
	printf("start: 0x%x, size: 0x%x\n", start, size);
	
	/* allocate A and B on the device */
	uint32_t size_buffer = 16; 
	eva_t A_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t));
	eva_t B_device = hb_mc_device_malloc(eva_id, size_buffer * sizeof(uint32_t));
	printf("A's EVA 0x%x, B's EVA: 0x%x\n", A_device, B_device);

	/* allocate A and B on the device */
	uint32_t A_host[size_buffer];
	uint32_t B_host[size_buffer];

	/* fill A and B */
	for (int i = 0; i < size_buffer; i++) {
		A_host[i] = i;
		B_host[i] = size_buffer - i - 1;
	}
	
	/* Copy A and B to device */	
	void *dst = (void *) A_device;
	void *src = (void *) &A_host[0];
	int error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device);
	if (error != HB_MC_SUCCESS) {
		printf("cosim_cuda_test(): could not copy buffer A to device.\n");
	}
	dst = (void *) B_device;
	src = (void *) &B_host[0];
	error = hb_mc_device_memcpy (fd, eva_id, dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_device);
	if (error != HB_MC_SUCCESS) {
		printf("cosim_cuda_test(): could not copy buffer B to device.\n");
	}

	// read back A and B from the device
	hb_mc_response_packet_t A_loads[size_buffer];
	hb_mc_response_packet_t B_loads[size_buffer];
	src = (void *) A_device;
	dst = (void *) &A_loads[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host);
	if (error != HB_MC_SUCCESS) {
		printf("cosim_cuda_test(): Unable to copy A from device.\n");
	}
	src = (void *) B_device;
	dst = (void *) &B_loads[0];
	error = hb_mc_device_memcpy (fd, eva_id, (void *) dst, src, size_buffer * sizeof(uint32_t), hb_mc_memcpy_to_host);
	if (error != HB_MC_SUCCESS) {
		printf("cosim_cuda_test(): Unable to copy B from device.\n");
	}
	/* print results */
	for (int i = 0; i < size_buffer; i++) {
		printf("A[%d]: write = 0x%x, read = 0x%x\n", i, A_host[i], hb_mc_response_packet_get_data(&A_loads[i]));
	}	
	printf("\n");
	for (int i = 0; i < size_buffer; i++) {
		printf("B[%d]: write = 0x%x, read = 0x%x\n", i, B_host[i], hb_mc_response_packet_get_data(&B_loads[i]));
	}		

	/* free A and B on device */
	/* TODO */

	hb_mc_device_finish(fd, eva_id, tiles, num_tiles);
	return;
}

#endif
