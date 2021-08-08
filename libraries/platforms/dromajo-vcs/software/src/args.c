// Copyright (c) 2020, University of Washington All rights reserved.
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

// This file includes routines that queries the arguments from the x86 host and 
// loads them into corresponding registers so that BlackParrot can access them.

#include <stdio.h>
#include <hb_bp_platform.h>
#include <bsg_manycore.h>
#include <bsg_manycore_packet.h>
#include <bsg_manycore_printing.h>

// Number of arguments that can be retrieved is currently based on the size of the buffer.
int hb_bp_argc;
char **hb_bp_argv;

/*
 * Retrieve arguments from the host
 */
void hb_bp_init_args(void) {
  int err;
  hb_mc_packet_t args_req_pkt, args_resp_pkt;

  static int hb_bp_arg_index = 0;
  static int hb_bp_char_index = 0;

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
    args_req_pkt.request.addr = HB_BP_HOST_EPA_ARGS_START + (hb_bp_arg_index << 2);
    err = hb_bp_write_to_mc_bridge(&args_req_pkt);
    err = hb_bp_read_from_mc_bridge(&args_resp_pkt, HB_MC_FIFO_RX_RSP);
    if (err != HB_MC_SUCCESS)
      bp_finish(err);

    uint32_t data = args_resp_pkt.response.data;
    uint8_t null_char = (uint8_t) '\0';
    if (data != HB_BP_HOST_OP_FINISH_CODE) {
      uint32_t mask = 0xFF000000;
      uint8_t byte;
      for(int i = 0; i < 4; i++) {
        // Get the correct byte from the 4-byte data and then shift it
        // to the lowest byte
        byte = ((data & mask) >> (8 * (3 - i)));
        if (byte != null_char) {
          buffer[hb_bp_char_index++] = (char) byte;
          mask >>= 8;
        }
        else {
          buffer[hb_bp_char_index++] = ' ';
          hb_bp_arg_index++;
          break;
        }
      }
    }
    else
      break;
  }
  hb_bp_argc = hb_bp_arg_index;

  // Convert a flat buffer into a 2-D array of pointers as argparse
  // expects it to be
  char *curr = buffer;
  char prev = ' ';
  int count = 0;
  if (hb_bp_argc != 0) {
    while (*curr != '\0') {
      if ((prev == ' ') && (prev != *curr)) {
        hb_bp_argv[count] = curr;
        count++;
      }
      prev = *curr;
      if (*curr == ' ')
        *curr = '\0';
      curr++;
    }
  }
}
