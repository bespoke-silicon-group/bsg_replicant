#pragma once
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
#include <errno.h>
#include <inttypes.h>

#include "bsg_manycore_driver.h"
#include "bsg_manycore_mem.h"
#include "bsg_manycore_loader.h"
#include "bsg_manycore_errno.h"

#include "graph.inc" // defines columns and row_ptrs

#define array_size(x)				\
    (sizeof(x)/sizeof(x[0]))

// converts sizeof to mc size in machine words
#define hb_mc_sizeof(x)                         \
        (sizeof(x)/sizeof(uint32_t))

static char ELF_CUDA_GRAPH_TEST_ENV [] = "elf_cuda_graph_rv32";
static char CUDA_GRAPH_KERNEL_NAME []  = "graph_degree_count";


static uint32_t results [array_size(row_ptrs)];

enum cuda_graph_argv_idx {
        COLUMNS_VECTOR = 0,
        COLUMNS_VECTOR_SZ,
        ROW_PTRS_VECTOR,
        ROW_PTRS_VECTOR_SZ,
        RESULTS_VECTOR,
        RESULTS_VECTOR_SZ,
} ;

void cosim_cuda_graph_degree_test ()
{
    uint8_t fd;

    hb_mc_init_host(&fd); // initalize the host

    tile_t tiles[] = {
        {
            .x = 0,
            .y = 1,
            .origin_x = 0,
            .origin_y = 1,
        }
    };

    eva_id_t eva_id = 0;
    uint32_t ntiles = array_size(tiles);

    char* fname = getenv(ELF_CUDA_GRAPH_TEST_ENV);
    if (!fname) {
	fprintf(stderr, "'%s' not defined: please defined '%s' to a RV32 binary to run this test\n",
		ELF_CUDA_GRAPH_TEST_ENV,
		ELF_CUDA_GRAPH_TEST_ENV);
	return;
    };

    FILE *file = fopen(fname, "rb");
    if (!file) {
	fprintf(stderr, "could not open '%s' defined by '%s': %s\n",
		fname,
		ELF_CUDA_GRAPH_TEST_ENV,
		strerror(errno));
	return;
    }
    fclose(file);

    if (hb_mc_init_device(fd, eva_id, fname, tiles, ntiles) != HB_MC_SUCCESS) {
	fprintf(stderr, "could not initialize device\n");
	return;
    }

    uint32_t start, size;
    _hb_mc_get_mem_manager_info(eva_id, &start, &size);
    printf("start: 0x%x, size: 0x%x\n", start, size); /* if CUDA init is correct, start should be TODO and size should be TODO */

    int err;
    eva_t columns_device, row_ptrs_device, results_device;
    err  = hb_mc_device_malloc(eva_id, sizeof(columns), &columns_device);
    if (err != HB_MC_SUCCESS) {
	fprintf(stderr, "failed to allocate column memory\n");
	goto test_exit;
    }

    err = hb_mc_device_malloc(eva_id, sizeof(row_ptrs), &row_ptrs_device);
    if (err != HB_MC_SUCCESS) {
	fprintf(stderr, "failed to allocate row memory\n");
	goto free_columns;
    }

    err = hb_mc_device_malloc(eva_id, sizeof(results), &results_device);
    if (err != HB_MC_SUCCESS) {
	fprintf(stderr, "failed to allocate results vector\n");
	goto free_device_resources;
    }

    /* copy input data */
    err = hb_mc_device_memcpy(fd, eva_id, (void*)columns_device, columns, sizeof(columns), hb_mc_memcpy_to_device);
    if (err != HB_MC_SUCCESS) {
	fprintf(stderr, "failed to copy columns to device\n");
	goto free_device_resources;
    }

    err = hb_mc_device_memcpy(fd, eva_id, (void*)row_ptrs_device, row_ptrs, sizeof(row_ptrs), hb_mc_memcpy_to_device);
    if (err != HB_MC_SUCCESS) {
	fprintf(stderr, "failed to copy row_ptrs to device\n");
	goto free_device_resources;
    }

    uint32_t argv [] = {
	[COLUMNS_VECTOR]    = (uint32_t) columns_device,
	[ROW_PTRS_VECTOR]   = (uint32_t) row_ptrs_device,
	[RESULTS_VECTOR]    = (uint32_t) results_device,
	[COLUMNS_VECTOR_SZ]    = (uint32_t) array_size(columns),
	[ROW_PTRS_VECTOR_SZ]   = (uint32_t) array_size(row_ptrs),
	[RESULTS_VECTOR_SZ]    = (uint32_t) array_size(results),
    };

    err = hb_mc_device_launch(fd, eva_id, CUDA_GRAPH_KERNEL_NAME, array_size(argv), argv, fname, tiles, ntiles);
    if (err != HB_MC_SUCCESS) {
        fprintf(stderr, "failed to launch kernel\n");
        goto free_device_resources;
    }

    hb_mc_cuda_sync(fd, &tiles[0]);

    /* copy the results from the device */
    err = hb_mc_device_memcpy(fd, eva_id, results, (void*)results_device, sizeof(results), hb_mc_memcpy_to_host);
    if (err != HB_MC_SUCCESS) {
        fprintf(stderr, "failed to copy results from device\n");
        goto free_device_resources;
    }

    printf("%s: Test PASSED!\n", __func__);


free_device_resources:
    hb_mc_device_free(eva_id, results_device);

free_row_ptrs:
    hb_mc_device_free(eva_id, row_ptrs_device);

free_columns:
    hb_mc_device_free(eva_id, columns_device);

test_exit:
    hb_mc_device_finish(fd, eva_id, tiles, ntiles);
    return;
}
