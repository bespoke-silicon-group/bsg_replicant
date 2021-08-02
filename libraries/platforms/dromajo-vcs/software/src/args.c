/*
 * args.c
 * This file includes routines that queries the arguments from the x86 host and 
 * loads them into corresponding registers so that BlackParrot can access them
 */

#include <hb_bp_platform.h>
#include <bsg_manycore.h>
#include <bsg_manycore_packet.h>
#include <stdio.h>
#include <bsg_manycore_printing.h>

// Number of arguments that can be retrieved is currently based on the size of the buffer.
// Other ideas (TODO):
// 1. Create pointers to locations of arguments on the stack (traditional method)
// 2. Flush the argument buffer periodically
int _argc;
char **_argv;
static int arg_index = 0;
static int char_index = 0;

/**
 * Retrieve arguments from the manycore host
 */
void __init_args(void) {
  int err;
  hb_mc_packet_t args_req_pkt, args_resp_pkt;

  // Flat buffer to capture all the arguments contiguously
  char buffer[3000];
  
  // Create a packet for the x86 host
  args_req_pkt.request.x_dst = HOST_X_COORD;
  args_req_pkt.request.y_dst = HOST_Y_COORD;
  args_req_pkt.request.x_src = BP_HOST_LINK_X;
  args_req_pkt.request.y_src = BP_HOST_LINK_Y;
  args_req_pkt.request.op_v2 = HB_MC_PACKET_OP_REMOTE_LOAD;
  args_req_pkt.request.payload = 0;
  args_req_pkt.request.reg_id = 0;

  while(1) {
    // The platform setup in simulation ensures that this packet will not go over the network so
    // we don't need to check for credits. Also, this code is executed before the CUDA-lite
    // program starts.
    args_req_pkt.request.addr = HB_BP_HOST_EPA_ARGS_START + (arg_index << 2);
    err = hb_bp_write_to_mc_bridge(&args_req_pkt);
    err = hb_bp_read_from_mc_bridge(&args_resp_pkt, HB_MC_FIFO_RX_RSP);
    if (err != HB_MC_SUCCESS)
      bp_finish(err);

    uint32_t data = args_resp_pkt.response.data;
    bsg_pr_info("Returned data: %x\n", data);
    uint8_t null_char = (uint8_t) '\0';
    if (data != HB_BP_HOST_OP_FINISH_CODE) {
      uint32_t mask = 0xFF000000;
      uint8_t byte;
      for(int i = 0; i < 4; i++) {
        // Get the correct byte from the 4-byte data and then shift it
        // to the lowest byte
        byte = ((data & mask) >> (8 * (3 - i)));
        bsg_pr_info("Argument %d at %x is %c\n", arg_index, char_index, (char) byte);
        if (byte != null_char) {
          buffer[char_index++] = (char) byte;
          mask >>= 8;
        }
        else {
          buffer[char_index++] = ' ';
          arg_index++;
          break;
        }
      }
    }
    else
      break;
  }
  _argc = arg_index;

  // Convert a flat buffer into a 2-D array of pointers as argparse
  // expects it to be
  char *curr = buffer;
  char prev = ' ';
  int count = 0;
  if (_argc != 0) {
    while (*curr != '\0') {
      if ((prev == ' ') && (prev != *curr)) {
        _argv[count] = curr;
        count++;
      }
      prev = *curr;
      if (*curr == ' ')
        *curr = '\0';
      curr++;
    }
  }

  // Debugging code:
  bsg_pr_info("Received arguments\n");
  bsg_pr_info("Number of arguments = %d\n", _argc);
  char **check_argv = _argv;
  int debug_count = 0;
  while (debug_count < _argc) {
    bsg_pr_info("Argument %d is %s\n", debug_count, check_argv[debug_count]);
    debug_count++;
  }
}
