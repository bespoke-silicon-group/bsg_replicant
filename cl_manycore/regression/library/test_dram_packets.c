#include "test_dram_packets.h"

// TODO: Make these not hard coded (for example, read them from ROM if that becomes available). 
#define NUM_SETS 64
#define ASSOCIATIVITY 2
#define CACHE_LINE_SIZE_WORDS 16
#define WORD_SIZE sizeof(uint32_t)
#define CACHE_LINE_SIZE (CACHE_LINE_SIZE_WORDS * WORD_SIZE)

int flush_cache_line_of(uint8_t fd, uint32_t addr, uint32_t target_x, uint32_t target_y)
{
        hb_mc_request_packet_t req;
        for(int i = 1; i <= ASSOCIATIVITY; i++)
        {
                uint32_t new_addr = (uint32_t)(((uint64_t)addr +  i * NUM_SETS * CACHE_LINE_SIZE) % (1 << 31)); 
                hb_mc_format_request_packet(fd, &req, new_addr, 0, target_x, target_y,  HB_MC_PACKET_OP_REMOTE_STORE);
                if(hb_mc_fifo_transmit(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&req)) != HB_MC_SUCCESS) {
                        bsg_pr_test_info(BSG_RED("Failed to write to FIFO\n"));
                        return HB_MC_FAIL;;
                }
        }
        return HB_MC_SUCCESS;
}

int test_address(uint8_t fd, uint32_t addr, uint32_t data, uint32_t target_x, uint32_t target_y) {
        hb_mc_request_packet_t req;
        hb_mc_response_packet_t res;

        hb_mc_format_request_packet(fd, &req, addr, data, target_x, target_y, HB_MC_PACKET_OP_REMOTE_STORE);
        
        bsg_pr_test_info("Sending packet to tile (%d, %d), address 0x%x\n", target_x, target_y, addr);
        if(hb_mc_fifo_transmit(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&req)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to write to FIFO\n"));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info(BSG_GREEN("Written successfully\n"));
        
        hb_mc_request_packet_set_op(&req, HB_MC_PACKET_OP_REMOTE_LOAD);
        bsg_pr_test_info("Testing reading packet. It should be in VCache.\n");
        if(hb_mc_fifo_transmit(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&req)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to write to FIFO\n"));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info("Reading\n");
        if(hb_mc_fifo_receive(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&res)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to read from FIFO\n"));
                return HB_MC_FAIL;
        }

        bsg_pr_test_info("Comparing data\n");
        uint32_t actual = hb_mc_response_packet_get_data(&res);
        if(actual != data) {
                bsg_pr_test_info(BSG_RED("Comparison failed: 0x%x != 0x%x\n"), data, actual);
                return HB_MC_FAIL;
        }

        
        bsg_pr_test_info("Flushing cache line\n");
        if(flush_cache_line_of(fd, addr, target_x, target_y) != HB_MC_SUCCESS)
                return HB_MC_FAIL;
        bsg_pr_test_info("Success\n");

        
        bsg_pr_test_info("Testing reading packet. It should be coming from DRAM now\n");
        
        if(hb_mc_fifo_transmit(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&req)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to write to FIFO\n"));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info("Reading\n");
        if(hb_mc_fifo_receive(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&res)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to read from FIFO\n"));
                return HB_MC_FAIL;
        }

        bsg_pr_test_info("Comparing data\n");
        actual = hb_mc_response_packet_get_data(&res);
        if(actual != data) {
                bsg_pr_test_info(BSG_RED("Comparison failed: 0x%x != 0x%x\n"), data, actual);
                return HB_MC_FAIL;
        }

        return HB_MC_SUCCESS;
}

int test_dram_packets() {
        uint8_t fd; 
      	if (hb_mc_fifo_init(&fd) != HB_MC_SUCCESS) {
		bsg_pr_test_info(BSG_RED("Failed to initialize fifo.\n"));
		return HB_MC_FAIL;
	}
 
        srand(time(0));
        
        uint32_t host_x, host_y, dim_x, dim_y;
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &host_x) == HB_MC_FAIL) {
                bsg_pr_test_info(BSG_RED("Failed to read host X coordinate from rom.\n"));
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &host_y) == HB_MC_FAIL) {
                bsg_pr_test_info(BSG_RED("Failed to read host Y coordinate from rom.\n"));
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_X, &dim_x) == HB_MC_FAIL) {
                bsg_pr_test_info(BSG_RED("Failed to read device X dimension from rom.\n"));
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_Y, &dim_y) == HB_MC_FAIL) {
                bsg_pr_test_info(BSG_RED("Failed to read device Y dimension from rom.\n"));
                return HB_MC_FAIL;
        }
        
        uint8_t target_x = dim_x + 1;
        uint8_t target_y = dim_y + 1;
        uint32_t min_addr = 0;
        uint32_t max_addr = 1 << 27 - 1;
        uint32_t data = rand();
        bsg_pr_test_info("Min Address: 0x%x, Max Address: 0x%x, Expected data: 0x%x\n", min_addr, max_addr, data);

        if(test_address(fd, min_addr, data, target_x, target_y) == HB_MC_FAIL ||
           test_address(fd, max_addr, data, target_x, target_y) == HB_MC_FAIL)
                return HB_MC_FAIL;

        
        return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_dram_packets Regression Test (COSIMULATION)\n");
	int rc = test_dram_packets();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_dram_packets Regression Test (F1)\n");
	int rc = test_dram_packets();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
