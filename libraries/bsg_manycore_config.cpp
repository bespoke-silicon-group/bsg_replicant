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

#ifdef __cplusplus
#include <cmath>
#else
#include <math.h>
#endif

static const char error_init_help [] = "Is your FPGA initialized with an AGFI?";

static bool is_power2(hb_mc_idx_t u)
{
        return __builtin_popcount(u) == 1;
}

int hb_mc_config_init(const hb_mc_config_raw_t raw[HB_MC_CONFIG_MAX],
                      hb_mc_config_t *config)
{
        uint32_t xlogsz_max, xdim_max;
        hb_mc_config_raw_t cur;
        hb_mc_idx_t idx;
        char date[8], version[3];

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
        if (idx > HB_MC_CONFIG_MAX_BITWIDTH_DATA){
                bsg_pr_err("%s: Invalid Network Datapath Bitwidth %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->network_bitwidth_data = idx;

        idx = raw[HB_MC_CONFIG_NETWORK_ADDR_WIDTH];
        if (idx > HB_MC_CONFIG_MAX_BITWIDTH_ADDR){
                bsg_pr_err("%s: Invalid Network Address Bitwidth %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->network_bitwidth_addr = idx;

        /* The maximum X dimension of the network is limited by the network
         * address bitwidth */
        xlogsz_max = HB_MC_CONFIG_MAX_BITWIDTH_ADDR - config->network_bitwidth_addr;
        xdim_max = (1 << xlogsz_max);

        idx = raw[HB_MC_CONFIG_DEVICE_DIM_X];
        //Temporarily removed this condition until it is cleared up. TODO: Fix.
        //if ((idx < HB_MC_COORDINATE_MIN) || (idx > xdim_max)){
        if ((idx < HB_MC_COORDINATE_MIN)){
                bsg_pr_err("%s: Invalid Device Dimension X %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->vcore_dimensions.x = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_DIM_Y];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > HB_MC_COORDINATE_MAX)){
                bsg_pr_err("%s: Invalid Device Dimension Y %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->vcore_dimensions.y = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > config->vcore_dimensions.x)){
                bsg_pr_err("%s: Invalid Host Interface index X %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->host_interface.x = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > config->vcore_dimensions.y)){
                bsg_pr_err("%s: Invalid Host Interface index Y %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->host_interface.y = idx;

        config->basejump = raw[HB_MC_CONFIG_REPO_BASEJUMP_HASH];
        config->manycore = raw[HB_MC_CONFIG_REPO_MANYCORE_HASH];
        config->f1 = raw[HB_MC_CONFIG_REPO_F1_HASH];

        /* set the victim cache parameters from the values in the ROM */
        config->vcache_ways         = raw[HB_MC_CONFIG_VCACHE_WAYS];
        config->vcache_sets         = raw[HB_MC_CONFIG_VCACHE_SETS];
        config->vcache_block_words  = raw[HB_MC_CONFIG_VCACHE_BLOCK_WORDS];

        idx = raw[HB_MC_CONFIG_VCACHE_STRIPE_WORDS];
        if (idx < config->vcache_block_words) {
                bsg_pr_err("%s: Invalid vcache stripe size %d: stripe size "
                           "cannot be smaller than vcache block size %d: %s\n",
                           __func__, idx, config->vcache_block_words, error_init_help);
                return HB_MC_INVALID;
        }
        config->vcache_stripe_words = idx;


        idx = raw[HB_MC_CONFIG_IO_REMOTE_LOAD_CAP];
        if ((idx < HB_MC_REMOTE_LOAD_MIN) || (idx > HB_MC_REMOTE_LOAD_MAX)){
                bsg_pr_err("%s: Invalid remote load caps %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->io_remote_load_cap = idx;


        idx = raw[HB_MC_CONFIG_IO_EP_MAX_OUT_CREDITS];
        if ((idx < HB_MC_EP_OUT_CREDITS_MIN) || (idx > HB_MC_EP_OUT_CREDITS_MAX)){
                bsg_pr_err("%s: Invalid endpoint max out credits %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->io_endpoint_max_out_credits = idx;


        idx = raw[HB_MC_CONFIG_IO_HOST_CREDITS_CAP];
        if ((idx < HB_MC_HOST_CREDITS_MIN) || (idx > HB_MC_HOST_CREDITS_MAX)){
                bsg_pr_err("%s: Invalid host credits CAP %d: %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->io_host_credits_cap = idx;

        idx = raw[HB_MC_CONFIG_DRAM_CHANNELS];
        if (!is_power2(idx) || idx > 32) {
                bsg_pr_err("%s: Invalid DRAM channels: %" PRIu32 ": %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->dram_channels = idx;

        idx = raw[HB_MC_CONFIG_DRAM_BANK_SIZE_WORDS];
        if (!is_power2(idx)) {
                bsg_pr_err("%s: Invalid DRAM Bank size: %" PRIu32 ": %s\n",
                           __func__, idx, error_init_help);
                return HB_MC_INVALID;
        }
        config->dram_bank_size_words = idx;

        if (hb_mc_memsys_get_id_from_rom_value(
                    raw[HB_MC_CONFIG_MEMSYS_ID],
                    &config->memsys_id)
            != HB_MC_SUCCESS)
                return HB_MC_INVALID;

        idx = raw[HB_MC_CONFIG_MEMSYS_FEATURE_DMA];
        if (idx != 0 && idx != 1) {
                bsg_pr_err("%s: Invalid value for DMA support: %s\n",
                           __func__,
                           error_init_help);
                return HB_MC_INVALID;
        }
        config->memsys_feature_dma = idx;

        idx = raw[HB_MC_CONFIG_MEMSYS_FEATURE_CACHE];
        if (idx != 0 && idx != 1) {
                bsg_pr_err("%s: Invalid value for cache existence: %s\n",
                           __func__,
                           error_init_help);
                return HB_MC_INVALID;
        }
        config->memsys_feature_cache = idx;

#define SET_CONFIG_FIELD(field, index, test_condition)                  \
        idx = raw[index];                                               \
        if (!(test_condition)) {                                        \
            bsg_pr_err("%s: Invalid " #field " : %" PRIu32 ": %s\n",    \
                       __func__, idx, error_init_help);                 \
            return HB_MC_INVALID;                                       \
        }                                                               \
        config->field = idx;

        SET_CONFIG_FIELD(memsys_dram_ro_bits,
                         HB_MC_CONFIG_MEMSYS_DRAM_RO_BITS,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_bg_bits,
                         HB_MC_CONFIG_MEMSYS_DRAM_BG_BITS,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_ba_bits,
                         HB_MC_CONFIG_MEMSYS_DRAM_BA_BITS,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_co_bits,
                         HB_MC_CONFIG_MEMSYS_DRAM_CO_BITS,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_byte_offset_bits,
                         HB_MC_CONFIG_MEMSYS_DRAM_BYTE_OFF_BITS,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_ro_bitidx,
                         HB_MC_CONFIG_MEMSYS_DRAM_RO_BITIDX,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_bg_bitidx,
                         HB_MC_CONFIG_MEMSYS_DRAM_BG_BITIDX,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_ba_bitidx,
                         HB_MC_CONFIG_MEMSYS_DRAM_BA_BITIDX,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_co_bitidx,
                         HB_MC_CONFIG_MEMSYS_DRAM_CO_BITIDX,
                         ((0 <= idx) && (idx <= 30)));

        SET_CONFIG_FIELD(memsys_dram_byte_offset_bitidx,
                         HB_MC_CONFIG_MEMSYS_DRAM_BYTE_OFF_BITIDX,
                         ((0 <= idx) && (idx <= 30)));


#undef SET_CONFIG_FIELD

        bsg_pr_dbg("%s: ro { bits = %llu, idx = %llu}\n",
                   __func__,
                   config->memsys_feature_dram_ro_bits,
                   config->memsys_feature_dram_ro_bitidx);

        bsg_pr_dbg("%s: bg { bits = %llu, idx = %llu}\n",
                   __func__,
                   config->memsys_feature_dram_bg_bits,
                   config->memsys_feature_dram_bg_bitidx);

        bsg_pr_dbg("%s: ba { bits = %llu, idx = %llu}\n",
                   __func__,
                   config->memsys_feature_dram_ba_bits,
                   config->memsys_feature_dram_ba_bitidx);

        bsg_pr_dbg("%s: co { bits = %llu, idx = %llu}\n",
                   __func__,
                   config->memsys_feature_dram_co_bits,
                   config->memsys_feature_dram_co_bitidx);

        bsg_pr_dbg("%s: bo { bits = %llu, idx = %llu}\n",
                   __func__,
                   config->memsys_feature_dram_byte_offset_bits,
                   config->memsys_feature_dram_byte_offset_bitidx);

        return HB_MC_SUCCESS;
}
