#ifndef BSG_MANYCORE_PLATFORM_HPP
#define BSG_MANYCORE_PLATFORM_HPP

#define HOST_BASE_ADDRESS 0x100000
#define HOST_PUTCHAR_REG  (HOST_BASE_ADDRESS + 0x1000)
#define HOST_GETCHAR_REG  (HOST_BASE_ADDRESS + 0x2000)
#define HOST_BOOTROM_REG  (HOST_BASE_ADDRESS + 0x3000)

class bp_mc_link_t {
    private:
                 uint64_t  fifo_base_addr        = (uint64_t) 0x500000;
        volatile uint64_t *bp_req_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x1000);
        volatile int      *bp_req_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x2000);
        volatile uint64_t *mc_rsp_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x3000);
        volatile int      *mc_rsp_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x4000);
        volatile uint64_t *mc_req_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x5000);
        volatile int      *mc_req_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x6000);
        volatile uint64_t *bp_rsp_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x7000);
        volatile int      *bp_rsp_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x8000);
        volatile int      *endpoint_credits_addr = (volatile int *) (fifo_base_addr + 0x9000);

        int try_write_bp_request_fifo(uint64_t data);
        int try_write_bp_response_fifo(uint64_t data);
        int try_read_mc_response_fifo(uint64_t *data);
        int try_read_mc_request_fifo(uint64_t *data);

    public:
        int tx_fifo_req(hb_mc_request_packet_t *packet);
        int tx_fifo_rsp(hb_mc_response_packet_t *packet);
        int rx_fifo_req(hb_mc_request_packet_t *packet);
        int rx_fifo_rsp(hb_mc_response_packet_t *packet);
        int mmio_read(uint64_t address, int32_t *data);
        int mmio_write(uint64_t address, int32_t data, uint8_t mask);
        int fifo_fence();
        int fifo_drain();
};

#endif

