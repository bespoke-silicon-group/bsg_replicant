#include <bsg_manycore_config.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>

#ifdef __cplusplus
#include <cmath>
#else
#include <math.h>
#endif

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
                bsg_pr_err("%s: Invalid Network Datapath Bitwidth %d\n",
			__func__, idx);
                return HB_MC_FAIL;
        }
        config->network_bitwidth_data = idx;

	idx = raw[HB_MC_CONFIG_NETWORK_ADDR_WIDTH];
        if (idx > HB_MC_CONFIG_MAX_BITWIDTH_ADDR){
                bsg_pr_err("%s: Invalid Network Address Bitwidth %d\n",
			__func__, idx);
                return HB_MC_FAIL;
        }
        config->network_bitwidth_addr = idx;

	/* The maximum X dimension of the network is limited by the network
	 * address bitwidth */
	xlogsz_max = HB_MC_CONFIG_MAX_BITWIDTH_ADDR - config->network_bitwidth_addr;
	xdim_max = (xlogsz_max - 1) << 1;

        idx = raw[HB_MC_CONFIG_DEVICE_DIM_X];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > xdim_max)){
                bsg_pr_err("%s: Invalid Device Dimension X %d\n",
			__func__, idx);
                return HB_MC_FAIL;
        }
        config->dimension.x = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_DIM_Y];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > HB_MC_COORDINATE_MAX)){
                bsg_pr_err("%s: Invalid Device Dimension Y %d\n",
			__func__, idx);
                return HB_MC_FAIL;
        }
        config->dimension.y = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > config->dimension.x)){
                bsg_pr_err("%s: Invalid Host Interface index X %d\n",
			__func__, idx);
                return HB_MC_FAIL;
        }
        config->host_interface.x = idx;

        idx = raw[HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y];
        if ((idx < HB_MC_COORDINATE_MIN) || (idx > config->dimension.y)){
                bsg_pr_err("%s: Invalid Host Interface index Y %d\n",
			__func__, idx);
                return HB_MC_FAIL;
        }
        config->host_interface.y = idx;

        config->basejump = raw[HB_MC_CONFIG_REPO_BASEJUMP_HASH];
        config->manycore = raw[HB_MC_CONFIG_REPO_MANYCORE_HASH];
        config->f1 = raw[HB_MC_CONFIG_REPO_F1_HASH];

        return HB_MC_SUCCESS;
}
