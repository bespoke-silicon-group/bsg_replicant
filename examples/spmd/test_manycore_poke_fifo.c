

#include <stdint.h>

typedef struct response_packet {
        uint8_t   x_dst; //!< x coordinate of the requester
        uint8_t   y_dst; //!< y coordinate of the requester
        uint8_t   load_id; //!< read response id
        uint32_t  data; //!< packet's payload data
        uint8_t   op;    //!< opcode
        uint8_t   reserved[8];
} __attribute__((packed)) hb_mc_response_packet_t;

typedef struct request_packet {
        uint8_t  x_dst; //!< x coordinate of the responder
        uint8_t  y_dst; //!< y coordinate of the responder
        uint8_t  x_src; //!< x coordinate of the requester
        uint8_t  y_src; //!< y coordinate of the requester
        uint32_t data;  //!< packet's payload data
        uint8_t  reg_id; //!< 5-bit id for load or amo
        uint8_t  op_ex;  //!< 4-bit byte mask
        uint8_t  op;    //!< opcode
        uint32_t addr;  //!< address field (EPA)
        uint8_t  reserved[1];
}  __attribute__((packed)) hb_mc_request_packet_t;

typedef union packet {
        hb_mc_request_packet_t request; /**/
        hb_mc_response_packet_t response; /* from the Hammerblade Manycore */
        uint64_t words[2];
} hb_mc_packet_t;

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

    uint64_t bp_cfg_offset = 0x200000;
    volatile uint64_t *did_mask_addr = (uint64_t *) (0x0009 + bp_cfg_offset);
    volatile uint64_t *sac_mask_addr = (uint64_t *) (0x000a + bp_cfg_offset);

    // Unlock manycore domain
    *did_mask_addr = 3;

    uint64_t  bp_coproc_offset             = 2UL << 36UL;

    volatile uint64_t *mc_link_bp_req_fifo_addr     = (uint64_t *) (0x01000 + bp_coproc_offset);
    volatile uint64_t *mc_link_bp_req_credits_addr  = (uint64_t *) (0x02000 + bp_coproc_offset);
    volatile uint64_t *mc_link_bp_resp_fifo_addr    = (uint64_t *) (0x03000 + bp_coproc_offset);
    volatile uint64_t *mc_link_bp_resp_entries_addr = (uint64_t *) (0x04000 + bp_coproc_offset);
    volatile uint64_t *mc_link_mc_req_fifo_addr     = (uint64_t *) (0x05000 + bp_coproc_offset);
    volatile uint64_t *mc_link_mc_req_entries_addr  = (uint64_t *) (0x06000 + bp_coproc_offset);
    volatile uint64_t *mc_link_rom_start_addr       = (uint64_t *) (0x07000 + bp_coproc_offset);
    volatile uint64_t *mc_link_rom_end_addr         = (uint64_t *) (0x07fff + bp_coproc_offset);

    // Set up EPA mapping according to vanilla core map
    uint32_t mc_csr_freeze_eva = 0x00020000UL;
    uint32_t mc_csr_pc_eva     = 0x0002000cUL;

    hb_mc_packet_t req_packet;

    // Store deadbeef to PC
    req_packet.request.x_dst    = 0;
    req_packet.request.y_dst    = 2;
    // Unused
    req_packet.request.x_src    = 0;
    // Unused
    req_packet.request.y_src    = 1;
    req_packet.request.data     = 0xdeadbeef;
    req_packet.request.reg_id   = 0x0;
    // Store mask
    req_packet.request.op_ex    = 0xf;
    req_packet.request.op       = 1; // Store
    req_packet.request.addr     = mc_csr_pc_eva;

    *mc_link_bp_req_fifo_addr = req_packet.words[0];
    *mc_link_bp_req_fifo_addr = req_packet.words[1];

    // Read deadbeef from PC
    req_packet.request.x_dst    = 0;
    req_packet.request.y_dst    = 2;
    // Unused
    req_packet.request.x_src    = 0;
    // Unused
    req_packet.request.y_src    = 1;
    req_packet.request.data     = 0;
    req_packet.request.reg_id   = 0xc;
    // Store mask
    req_packet.request.op_ex    = 0;
    req_packet.request.op       = 0; // Load
    req_packet.request.addr     = mc_csr_pc_eva;

    *mc_link_bp_req_fifo_addr = req_packet.words[0];
    *mc_link_bp_req_fifo_addr = req_packet.words[1];

    // Wait for response
    while (*mc_link_bp_resp_entries_addr == 0);

    hb_mc_packet_t resp_packet;

    resp_packet.words[0] = *mc_link_bp_resp_fifo_addr;
    resp_packet.words[1] = *mc_link_bp_resp_fifo_addr;

    while(1);
}

