#include "test_manycore_compile.h"

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

static
void print_config(hb_mc_manycore_t *manycore)
{
        bsg_pr_test_info("Printing configuration for '%s'\n", manycore->name);
        bsg_pr_test_info("%s: basejump-stl: 0x%" PRIx32 "\n",
                         manycore->name, hb_mc_config_get_githash_basejump(&manycore->config));
        bsg_pr_test_info("%s: manycore:     0x%" PRIx32 "\n",
                         manycore->name, hb_mc_config_get_githash_manycore(&manycore->config));
        bsg_pr_test_info("%s: f1:           0x%" PRIx32 "\n",
                         manycore->name, hb_mc_config_get_githash_f1(&manycore->config));
}

static
int test_manycore_compile(void)
{
        int rc;
        hb_mc_manycore_t manycore;
        rc = hb_mc_manycore_init(&manycore, "manycore@test_manycore_compile", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device!\n");
                return HB_MC_FAIL;
        }

        uintptr_t addr = hb_mc_mmio_fifo_get_reg_addr(HB_MC_FIFO_RX_REQ,
                                                      HB_MC_MMIO_FIFO_ISR_OFFSET);
        uint32_t val;
        hb_mc_manycore_mmio_read32(&manycore, addr, &val);
        bsg_pr_test_info("Value @ 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", addr, val);

        print_config(&manycore);
        return 0;
}
#ifdef COSIM
void test_main(uint32_t *exit_code) {   
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
        bsg_pr_test_info("test_manycore_compile Regression Test (COSIMULATION)\n");
        int rc = test_manycore_compile();
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main() {
        bsg_pr_test_info("test_manycore_compile Regression Test (F1)\n");
        int rc = test_manycore_compile();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif
