#include "test_manycore_eva.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

typedef enum __hb_mc_region_t{
	LOCAL = 0,
	GROUP = 1,
	GLOBAL = 2,
	DRAM = 3,
	NUM_REGIONS = 4
} hb_mc_region_t;

#define NETWORK_ADDRESS_BITS 32
#define NUM_REGIONS 4 // Local, Group, Global, DRAM
#define NUM_EVAS 100
#define NUM_NPAS 100

// These are explicitly not copied from the bsg_manycore_eva.h 
#define LOCAL_EPA_SIZE (1<<18)

#define GROUP_INDICATOR (1 << 29)
#define GROUP_EPA_SIZE LOCAL_EPA_SIZE
#define GROUP_X_BITS 6
#define GROUP_X_MAX ((1 << GROUP_X_BITS) - 1)
#define GROUP_X_MASK GROUP_X_MAX
#define GROUP_X_OFFSET 18

#define GROUP_Y_BITS 5
#define GROUP_Y_MAX ((1 << GROUP_Y_BITS) - 1)
#define GROUP_Y_MASK GROUP_Y_MAX
#define GROUP_Y_OFFSET (GROUP_X_OFFSET + GROUP_X_BITS)

#define GLOBAL_INDICATOR_WIDTH 1
#define GLOBAL_INDICATOR (1 << 30)
#define GLOBAL_EPA_SIZE LOCAL_EPA_SIZE
#define GLOBAL_X_BITS 6
#define GLOBAL_X_MAX ((1 << GLOBAL_X_BITS) - 1)
#define GLOBAL_X_MASK GLOBAL_X_MAX
#define GLOBAL_X_OFFSET 18

#define GLOBAL_Y_BITS 6
#define GLOBAL_Y_MAX ((1 << GLOBAL_Y_BITS) - 1)
#define GLOBAL_Y_MASK GLOBAL_Y_MAX
#define GLOBAL_Y_OFFSET (GLOBAL_X_OFFSET + GLOBAL_X_BITS)

#define DRAM_INDICATOR_WIDTH 1
#define DRAM_INDICATOR (1 << 31)
#define DRAM_EPA_SIZE 16384

int test_manycore_eva () {
        int rc = 0, fail = 0, i = 0;
	uint8_t dram_x_offset;
	hb_mc_region_t region;
	hb_mc_npa_t npa;
	size_t npa_sz;
	hb_mc_epa_t epa;
	hb_mc_eva_t eva;
        hb_mc_dimension_t dim;
	hb_mc_idx_t dim_x, dim_y;

        hb_mc_coordinate_t src;
	hb_mc_idx_t src_x, src_y;

        hb_mc_coordinate_t tgt;
	hb_mc_idx_t tgt_x, tgt_y;
	hb_mc_epa_t tgt_epa;
	size_t tgt_sz;

	hb_mc_npa_t result_npa;
	hb_mc_epa_t result_epa;
        hb_mc_coordinate_t result_c;
	hb_mc_idx_t result_x, result_y;
	size_t result_sz;

        const hb_mc_config_t *config;
        hb_mc_manycore_t mc = {0};

	srand(42);

        rc = hb_mc_manycore_init(&mc, "manycore@test_manycore_eva", 0);
        if(rc != HB_MC_SUCCESS){
		bsg_pr_test_err("Failed to initialize manycore device: %s\n",
				hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);
        dim = hb_mc_config_get_dimension(config);
	dim_x = hb_mc_dimension_get_x(dim);
	dram_x_offset = NETWORK_ADDRESS_BITS - DRAM_INDICATOR_WIDTH - ceil(log2(dim_x));

	if(dim_x > GROUP_X_MAX){
		bsg_pr_test_err("Manycore X dimension is larger than GROUP_X_MAX");
                return HB_MC_FAIL;
	}
	if(dim_x > GLOBAL_X_MAX){
		bsg_pr_test_err("Manycore X dimension is larger than GLOBAL_X_MAX");
                return HB_MC_FAIL;
	}
	dim_y = hb_mc_dimension_get_y(dim);
	if(dim_y > GROUP_Y_MAX){
		bsg_pr_test_err("Manycore Y dimension is larger than GROUP_Y_MAX");
                return HB_MC_FAIL;
	}
	if(dim_y > GLOBAL_Y_MAX){
		bsg_pr_test_err("Manycore Y dimension is larger than GLOBAL_Y_MAX");
                return HB_MC_FAIL;
	}

	bsg_pr_test_info("Testing Random Manycore EVAs\n");
	src_x = 1;
	src_y = 1;
	src = hb_mc_coordinate(src_x, src_y);
	for( i = 0; i < NUM_EVAS; ++i){
		region = rand() % NUM_REGIONS;
		if(region == LOCAL){
			eva = rand() % LOCAL_EPA_SIZE;
			bsg_pr_test_info("Creating Local EVA: 0x%x\n", eva);
			tgt_epa = eva;
			tgt_sz = LOCAL_EPA_SIZE - tgt_epa;
			tgt_x = src_x;
			tgt_y = src_y;
		} else if (region == GROUP){
			tgt_x = rand() % dim_x;
			tgt_y = rand() % (dim_y / 2);
			tgt_epa = rand() % GROUP_EPA_SIZE;
			eva = (GROUP_INDICATOR) | (tgt_x << GROUP_X_OFFSET) | 
				(tgt_y << GROUP_Y_OFFSET) | (tgt_epa);
			bsg_pr_test_info("Creating GROUP EVA: 0x%x\n", eva);
			tgt_sz = GROUP_EPA_SIZE - tgt_epa;
		} else if (region == GLOBAL){
			tgt_x = rand() % dim_x;
			tgt_y = rand() % dim_y;
			tgt_epa = rand() % GLOBAL_EPA_SIZE;
			eva = (GLOBAL_INDICATOR) | (tgt_x << GLOBAL_X_OFFSET) |
				(tgt_y << GLOBAL_Y_OFFSET) | (tgt_epa);
			bsg_pr_test_info("Creating Global EVA: 0x%x\n", eva);
			tgt_sz = GLOBAL_EPA_SIZE - tgt_epa;
		} else if (region == DRAM){
			tgt_x = rand() % dim_x;
			tgt_y = dim_y + 1;
			tgt_epa = rand() % DRAM_EPA_SIZE; // Small, but we'll deal.
			eva = DRAM_INDICATOR | (tgt_x << dram_x_offset) | (tgt_epa);
			bsg_pr_test_info("Creating DRAM EVA: 0x%x\n", eva);
			tgt_sz = (1 << dram_x_offset) - tgt_epa;
		} else {
			bsg_pr_test_err("Invalid Region number %d\n", region);
			return HB_MC_FAIL;
		}
		
		result_x = 0; result_y = 0; result_epa = 0; result_sz = 0;
		rc = hb_mc_eva_to_npa(config, &default_eva, &src,
					&eva, &result_npa, &result_sz);
		if(rc != HB_MC_SUCCESS){
			bsg_pr_test_err("Call to hb_mc_manycore_eva_to_npa failed\n", eva);
			fail = 1;
		}

		result_x = hb_mc_npa_get_x(&result_npa);
		result_y = hb_mc_npa_get_y(&result_npa);
		result_epa = hb_mc_npa_get_epa(&result_npa);

		if(result_x != tgt_x){
			bsg_pr_test_err("EVA To NPA: Unexpected NPA X Coordinate. Expected %d, got %d\n", 
					tgt_x, result_x);
			fail = 1;
		}

		if(result_y != tgt_y){
			bsg_pr_test_err("EVA To NPA: Unexpected NPA Y Coordinate. Expected %d, got %d\n", 
					tgt_y, result_y);
			fail = 1;
		}

		if(result_epa != tgt_epa){
			bsg_pr_test_err("EVA To NPA: Unexpected NPA EPA. Expected 0x%x, got 0x%x\n", 
					tgt_epa, result_epa);
			fail = 1;
		}

		if(result_sz != tgt_sz){
			bsg_pr_test_err("EVA To NPA: Unexpected NPA SZ. Expected 0x%x, got 0x%x\n", 
					tgt_sz, result_sz);
			fail = 1;
		}
	}

	bsg_pr_test_info("Creating Random Manycore NPAs\n");
	for( i = 0; i < NUM_NPAS; ++i){
	}

        hb_mc_manycore_exit(&mc);

        return fail ? HB_MC_FAIL : HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {
        bsg_pr_test_info("test_manycore_eva Regression Test (COSIMULATION)\n");
        int rc = test_manycore_eva();
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main() {
        bsg_pr_test_info("test_manycore_eva Regression Test (F1)\n");
        int rc = test_manycore_eva();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif

