#include "test_packets.h"

//TODO(sasha): Make this const once accessor api is fixed in bsg_manycore_packet
void request_packet_to_array(/*const*/ hb_mc_request_packet_t *pack, /*out*/ uint32_t *arr) {
        *arr++ = hb_mc_request_packet_get_x_dst(pack);
        *arr++ = hb_mc_request_packet_get_y_dst(pack);
        *arr++ = hb_mc_request_packet_get_x_src(pack);
        *arr++ = hb_mc_request_packet_get_y_src(pack);
        *arr++ = hb_mc_request_packet_get_mask (pack);
        *arr++ = hb_mc_request_packet_get_op   (pack);
        *arr++ = hb_mc_request_packet_get_addr (pack);
        *arr++ = hb_mc_request_packet_get_data (pack);
}

void response_packet_to_array(/*const*/ hb_mc_response_packet_t *pack, /*out*/ uint32_t *arr) {
        //NOTE(sasha): Does not test load_id. Not sure if it should,
        //             comment in hb_mc_response_packet_t says it
        //             is unused
        
        *arr++ = hb_mc_response_packet_get_x_dst(pack);
        *arr++ = hb_mc_response_packet_get_y_dst(pack);
        *arr++ = hb_mc_response_packet_get_op   (pack);
        *arr++ = hb_mc_response_packet_get_data (pack);
}

int test_packets() {
        
	uint8_t fd; 
	hb_mc_init_host(&fd);	
      	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		bsg_pr_test_info(BSG_RED("Failed to initialize host.\n"));
		return HB_MC_FAIL;
	}
 
        srand(time(0));
        
        uint32_t host_x, host_y;
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &host_x) == HB_MC_FAIL) {
                bsg_pr_test_info(BSG_RED("Failed to read host X coordinate from rom.\n"));
                return HB_MC_FAIL;
        }
        if(hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &host_y) == HB_MC_FAIL) {
                bsg_pr_test_info(BSG_RED("Failed to read host Y coordinate from rom.\n"));
                return HB_MC_FAIL;
        }

        uint8_t target_x = 0;
        uint8_t target_y = 1;
        uint32_t addr = DMEM_BASE >> 2;
        uint32_t data = rand();
        bsg_pr_test_info("Address: %x, Expected data: %x\n", addr, data);

        hb_mc_request_packet_t req1, req2;
        hb_mc_response_packet_t res;
        
        const char *req_desc[] = {
                "Destination X", "Destination Y",
                     "Source X",      "Source Y",
                         "Mask",        "Opcode",
                      "Address",          "Data",
        };

        uint32_t req_expected[] = {
                                      target_x,                     target_y,
                                        host_x,                       host_y,
                HB_MC_PACKET_REQUEST_MASK_WORD, HB_MC_PACKET_OP_REMOTE_STORE,
                                          addr,                         data,
        };

        const char *res_desc[] = {
                "Destination X", "Destination Y",
                       "Opcode",          "Data",
        };

        uint32_t res_expected[] = {
                                      host_x, host_y,
                HB_MC_PACKET_OP_REMOTE_STORE,   data,
        };

        uint32_t actual[8];
        
        bsg_pr_test_info("Manually formatting packet\n");
        hb_mc_request_packet_set_x_dst(&req1, target_x);
        hb_mc_request_packet_set_y_dst(&req1, target_y);
        hb_mc_request_packet_set_x_src(&req1, host_x);
        hb_mc_request_packet_set_y_src(&req1, host_y);
        hb_mc_request_packet_set_data (&req1, data);
        hb_mc_request_packet_set_mask (&req1, 0xF);
        hb_mc_request_packet_set_op   (&req1, HB_MC_PACKET_OP_REMOTE_STORE);
        hb_mc_request_packet_set_addr (&req1, addr);
        
        request_packet_to_array(&req1, actual);
        if(compare_results(8, req_desc, req_expected, actual) == HB_MC_FAIL)
                return HB_MC_FAIL;
        
        bsg_pr_test_info("Testing hb_mc_format_request_packet\n");
        hb_mc_format_request_packet(&req2, addr, data, target_x, target_y, HB_MC_PACKET_OP_REMOTE_STORE);
        request_packet_to_array(&req2, actual);
        if(compare_results(8, req_desc, req_expected, actual) == HB_MC_FAIL)
                return HB_MC_FAIL;

        bsg_pr_test_info("The two packets should be equal now:");
        if(hb_mc_request_packet_equals(&req1, &req2) == HB_MC_FAIL) {
                printf(BSG_RED("hb_mc_request_packet_equals says they are not\n"));
                return HB_MC_FAIL;
        }
        printf(BSG_GREEN("Success\n"));

        bsg_pr_test_info("Sending packet to tile (%d, %d)\n", target_x, target_y);
        if(hb_mc_fifo_transmit(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&req1)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to write to FIFO\n"));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info(BSG_GREEN("Written successfully\n"));
        
        hb_mc_request_packet_set_op(&req1, HB_MC_PACKET_OP_REMOTE_LOAD);
        bsg_pr_test_info("Testing reading packet\n");
        if(hb_mc_fifo_transmit(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&req1)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to write to FIFO\n"));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info("Reading\n");
        if(hb_mc_fifo_receive(fd, HB_MC_MMIO_FIFO_TO_DEVICE, (hb_mc_packet_t *)(&res)) != HB_MC_SUCCESS) {
                bsg_pr_test_info(BSG_RED("Failed to read from FIFO\n"));
                return HB_MC_FAIL;
        }
        bsg_pr_test_info("Comparing to expected read packet:\n");
        response_packet_to_array(&res, actual);
        if(compare_results(4, res_desc, res_expected, actual) == HB_MC_FAIL)
                return HB_MC_FAIL;
        return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_packets Regression Test (COSIMULATION)\n");
	int rc = test_packets();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_packets Regression Test (F1)\n");
	int rc = test_packets();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

