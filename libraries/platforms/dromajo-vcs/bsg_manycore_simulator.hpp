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

// This file implements the SimulationWrapper object.

#ifndef __BSG_MANYCORE_SIMULATOR_HPP
#define __BSG_MANYCORE_SIMULATOR_HPP

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <string>

#include <dromajo_cosim.h>
#include <dromajo_manycore.h>

#include <bsg_nonsynth_dpi_manycore.hpp>
#include <bsg_nonsynth_dpi_errno.hpp>
#include <bsg_nonsynth_dpi_cycle_counter.hpp>
#include <bsg_nonsynth_dpi_clock_gen.hpp>

#include <bsg_manycore.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_printing.h>

#include <bsg_manycore_regression.h>

#include <hb_bp_platform.h>

// Define invisible library functions
declare_hb_mc_get_bits
declare_bsg_printing

#ifndef NUM_DROMAJO_INSTR_PER_TICK
#define NUM_DROMAJO_INSTR_PER_TICK 1000
#endif

#ifndef NUM_TX_FIFO_CHK_PER_TICK
#define NUM_TX_FIFO_CHK_PER_TICK 100
#endif

class SimulationWrapper{
  // This is the generic pointer for implementation-specific
  // simulator details. In Verilator, this is
  // Vmanycore_tb_top. In VCS this is the scope
  // for DPI.
  void *top = nullptr;
  std::string *root;
  // Pointer to the dromajo instance
  dromajo_cosim_state_t *dromajo;
  // Pointer to the manycore DPI controller
  bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX> *dpi;
  // Fields to keep track of arguments
  int argc;
  char **argv;
  int char_index;

public:
  SimulationWrapper();
  ~SimulationWrapper();

  // Set the argument fields to pass them into Dromajo
  void set_args(int argc, char **argv);

  // Change the assertion state. 

  // When Verilator simulation starts, we want to disable
  // assertion because it is a two-state simulator and the lack
  // of z/x may cause erroneous assertions.
  //
  // This does not need to be implemented in 4-state simulators
  // like VCS
  void assertOn(bool val);
  
  std::string getRoot();

  // Advances to the next clock edge
  void advance_time();
  // Advances time by N clock edges
  void advance_time_cycles(int N);

  // DPI functions
  int dpi_fifo_drain(hb_mc_fifo_rx_t type);
  void dpi_cleanup();
  int dpi_init();

  // Dromajo functions
  int dromajo_init();
  bool dromajo_step();
  int dromajo_transmit_packet();
  int dromajo_transmit_host_packet(hb_mc_packet_t *dromajo_to_host_packet);
  int dromajo_receive_packet();
  int dromajo_update_credits();

  // Evaluates a maximum of NUM_DROMAJO_INSTR_PER_TICK instructions
  // in RISC-V and forwards packets between Dromajo and the manycore
  int eval();
};
#endif // __BSG_MANYCORE_SIMULATOR_HPP
