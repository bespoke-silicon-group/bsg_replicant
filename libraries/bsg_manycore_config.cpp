// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <bsg_manycore_config.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_chip_id.h>

#ifdef __cplusplus
#include <cmath>
#else
#include <math.h>
#endif

static const char error_init_help [] = "Is your FPGA initialized with an AGFI?";

// perform additional checks on the memory system
static
int hb_mc_config_init_check_memsys(hb_mc_config_t *config)
{
        if (config->memsys.id == HB_MC_MEMSYS_ID_INFMEM) {
                uint32_t dram_cos = hb_mc_config_get_num_dram_coordinates(config);
                uint32_t dram_chs = config->memsys.dram_channels;
                if (dram_cos != dram_chs) {
                        bsg_pr_err("%s: %s has %" PRIu32 " DRAM channels - but %" PRIu32 " expected\n",
                                   __func__,
                                   hb_mc_memsys_id_to_string(config->memsys.id),
                                   dram_chs,
                                   dram_cos);
                        return HB_MC_INVALID;
                }
        }
        return HB_MC_SUCCESS;
}

#define CHECK_FIELD(field, cond)                                        \
    do {                                                                \
        if (!(cond)) {                                                  \
            bsg_pr_err("%s: Configuration field %s: Condition '%s' failed: %s\n", \
                       __func__, #field, #cond, error_init_help);       \
            return HB_MC_INVALID;                                       \
        }                                                               \
    } while(0)

int hb_mc_config_init(const hb_mc_config_raw_t raw[HB_MC_CONFIG_MAX],
                      hb_mc_config_t *config)
{
        uint32_t xlogsz_max, xdim_max;
        hb_mc_config_raw_t cur;
        hb_mc_idx_t idx;
        char date[8], version[3];
        int err;

        /* Parse the Version */
        cur = raw[HB_MC_CONFIG_VERSION];
        version[2] = ((cur >> 16) & 0xFF);
        version[1] = ((cur >>  8) & 0xFF);
        version[0] = ((cur >>  0) & 0xFF);

        config->design_version.revision = version[0];
        config->design_version.minor = version[1];
        config->design_version.major = version[2];

        /* Parse the Compilation Date */
        cur = raw[HB_MC_CONFIG_TIMESTAMP];
        date[7] = ((cur >> 28) & 0xF);
        date[6] = ((cur >> 24) & 0xF);
        date[5] = ((cur >> 20) & 0xF);
        date[4] = ((cur >> 16) & 0xF);
        date[3] = ((cur >> 12) & 0xF);
        date[2] = ((cur >>  8) & 0xF);
        date[1] = ((cur >>  4) & 0xF);
        date[0] = ((cur >>  0) & 0xF);

        config->timestamp.year = date[3] * 1000 + date[2] * 100 + date[1] * 10 + date[0] * 1;
        config->timestamp.day =  date[5] * 10 + date[4] * 1;
        config->timestamp.month = date[7] * 10 + date[6] * 1;

        idx = raw[HB_MC_CONFIG_NETWORK_DATA_WIDTH];
        CHECK_FIELD(HB_MC_CONFIG_NETWORK_DATA_WIDTH, idx <= HB_MC_CONFIG_MAX_BITWIDTH_DATA);
        config->network_bitwidth_data = idx;

        idx = raw[HB_MC_CONFIG_NETWORK_ADDR_WIDTH];
        CHECK_FIELD(HB_MC_CONFIG_NETWORK_ADDR_WIDTH, idx <= HB_MC_CONFIG_MAX_BITWIDTH_ADDR);
        config->network_bitwidth_addr = idx;

        /* The maximum X dimension of the network is limited by the network
         * address bitwidth */
        xlogsz_max = HB_MC_CONFIG_MAX_BITWIDTH_ADDR - config->network_bitwidth_addr;
        xdim_max = (1 << xlogsz_max);

        idx = raw[HB_MC_CONFIG_POD_DIM_X];
        CHECK_FIELD(HB_MC_CONFIG_POD_DIM_X, idx >= HB_MC_COORDINATE_MIN);
        config->pod_shape.x = idx;

        idx = raw[HB_MC_CONFIG_POD_DIM_Y];
        CHECK_FIELD(HB_MC_CONFIG_POD_DIM_Y, idx >= HB_MC_COORDINATE_MIN && idx <= HB_MC_COORDINATE_MAX);
        config->pod_shape.y = idx;

        idx = raw[HB_MC_CONFIG_DIM_PODS_X];
        CHECK_FIELD(HB_MC_CONFIG_DIM_PODS_X, idx >= 0 && idx <= 64);
        config->pods.x = idx;

        idx = raw[HB_MC_CONFIG_DIM_PODS_Y];
        CHECK_FIELD(HB_MC_CONFIG_DIM_PODS_Y, idx >= 0 && idx <= 64);
        config->pods.y = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X];
        CHECK_FIELD(HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, idx >= 0 && idx <= 64);
        config->host_interface.x = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y];
        CHECK_FIELD(HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, idx >= 0 && idx <= 64);
        config->host_interface.y = idx;

        idx = raw[HB_MC_CONFIG_ORIGIN_COORD_X];
        CHECK_FIELD(HB_MC_CONFIG_ORIGIN_COORD_X, idx >= 0 && idx <= 64);
        config->origin.x = idx;

        idx = raw[HB_MC_CONFIG_ORIGIN_COORD_Y];
        CHECK_FIELD(HB_MC_CONFIG_ORIGIN_COORD_Y, idx >= 0 && idx <= 64);
        config->origin.y = idx;

        idx = raw[HB_MC_CONFIG_NOC_COORD_X_WIDTH];
        CHECK_FIELD(HB_MC_CONFIG_NOC_COORD_X_WIDTH, idx > 0 && idx < 32);
        config->noc_coord_width.x = idx;

        idx = raw[HB_MC_CONFIG_NOC_COORD_Y_WIDTH];
        CHECK_FIELD(HB_MC_CONFIG_NOC_COORD_Y_WIDTH, idx > 0 && idx < 32);
        config->noc_coord_width.y = idx;

        idx = raw[HB_MC_CONFIG_NOC_RUCHE_FACTOR_X];
        CHECK_FIELD(HB_MC_CONFIG_NOC_RUCHE_FACTOR_X, idx >= 0 && idx <= 3);
        config->noc_ruche_factor.x = idx;
        config->noc_ruche_factor.y = 0;

        idx = raw[HB_MC_CONFIG_BARRIER_RUCHE_FACTOR_X];
        CHECK_FIELD(HB_MC_CONFIG_RUCHE_FACTOR_X, idx >= 0 && idx <= 3);
        config->bar_ruche_factor.x = idx;
        config->bar_ruche_factor.y = 0;

        config->basejump = raw[HB_MC_CONFIG_REPO_BASEJUMP_HASH];
        config->manycore = raw[HB_MC_CONFIG_REPO_MANYCORE_HASH];
        config->f1 = raw[HB_MC_CONFIG_REPO_F1_HASH];
        config->chip_id = raw[HB_MC_CONFIG_CHIP_ID];
        CHECK_FIELD(HB_MC_CONFIG_CHIP_ID, HB_MC_IS_CHIP_ID(config->chip_id));

        /* set the victim cache parameters from the values in the ROM */
        config->vcache_ways         = raw[HB_MC_CONFIG_VCACHE_WAYS];
        config->vcache_sets         = raw[HB_MC_CONFIG_VCACHE_SETS];
        config->vcache_block_words  = raw[HB_MC_CONFIG_VCACHE_BLOCK_WORDS];

        idx = raw[HB_MC_CONFIG_VCACHE_STRIPE_WORDS];
        CHECK_FIELD(HB_MC_CONFIG_VCACHE_STRIPE_WORDS, idx >= config->vcache_block_words);
        config->vcache_stripe_words = idx;

        // Response fifo capacity
        idx = raw[HB_MC_CONFIG_IO_REMOTE_LOAD_CAP];
        CHECK_FIELD(HB_MC_CONFIG_IO_REMOTE_LOAD_CAP, idx >= HB_MC_REMOTE_LOAD_MIN && idx <= HB_MC_REMOTE_LOAD_MAX);
        config->io_remote_load_cap = idx;

        // Field no longer used
        idx = raw[HB_MC_CONFIG_IO_EP_MAX_OUT_CREDITS];
        //CHECK_FIELD(HB_MC_CONFIG_IO_EP_MAX_OUT_CREDITS, idx >= HB_MC_EP_OUT_CREDITS_MIN && idx <= HB_MC_EP_OUT_CREDITS_MAX);
        config->io_endpoint_max_out_credits = idx;

        // Host endpoint credits
        idx = raw[HB_MC_CONFIG_IO_HOST_CREDITS_CAP];
        CHECK_FIELD(HB_MC_CONFIG_IO_HOST_CREDITS_CAP, idx >= HB_MC_HOST_CREDITS_MIN && idx <= HB_MC_HOST_CREDITS_MAX);
        config->io_host_credits_cap = idx;

        err = hb_mc_memsys_init(&raw[HB_MC_CONFIG_MEMSYS], &config->memsys);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: Failed to initialize memory system: %s\n",
                           __func__, error_init_help);
                return err;
        }

        // Derived variables from the ROM
        config->tile_coord_width = hb_mc_coordinate(
            log2(config->pod_shape.x),
            log2(config->pod_shape.y)
            );
        config->pod_coord_width  = hb_mc_coordinate(
            config->noc_coord_width.x - config->tile_coord_width.x,
            config->noc_coord_width.y - config->tile_coord_width.y
            );

        return hb_mc_config_init_check_memsys(config);
}
