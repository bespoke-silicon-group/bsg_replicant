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

// This file implements the SimulatorWrapper object, which hides
// differences in simulators.

#include <bsg_manycore_simulator.hpp>
#include <svdpi.h>
// We use __m128i so that we can pass a 128-bit type between Verilog
// and C.
#include <xmmintrin.h>

// DPI function to advance time
extern "C" {
  void bsg_dpi_next();
}

////////////////////////////// SimulationWrapper functions //////////////////////////////

// Constructor
SimulationWrapper::SimulationWrapper() {
  root = new std::string("replicant_tb_top");
  std::string mc_dpi = *root + ".mc_dpi";
  top = svGetScopeFromName(mc_dpi.c_str());
  dpi = new bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX>(mc_dpi);

  // Initialize DPI and Dromajo
  dromajo_init();
  dpi_init();

  if (!dromajo) {
    bsg_pr_err("%s: Failed to initialize Dromajo pointer\n", __func__);
  }

  if (!dpi) {
    bsg_pr_err("%s: Failed to initialize DPI pointer\n", __func__);
  }

  // Initialize the argument character counter
  char_index = 0;
}

// Destructor
SimulationWrapper::~SimulationWrapper() {
  dpi_cleanup();
  dromajo_cosim_fini(dromajo);
  this->top = nullptr;
}

// Sets the arguments
void SimulationWrapper::set_args(int argc, char **argv) {
  this->argc = argc;
  this->argv = argv;
}

// Advances time to the next clock edge
void SimulationWrapper::advance_time() {
  svScope prev;
  prev = svSetScope(top);
  bsg_dpi_next();
  svSetScope(prev);
}

// Advances time by N clock edges
void SimulationWrapper::advance_time_cycles(int N) {
  svScope prev;
  prev = svSetScope(top);
  for (int i = 0; i < N; i++)
    bsg_dpi_next();
  svSetScope(prev);
}

// Does nothing. Turning on/off assertions is only supported in
// Verilator.
void SimulationWrapper::assertOn(bool val){

}

std::string SimulationWrapper::getRoot(){
  return *root;
}

/*
 * dpi_cleanup
 * Destroys the pointer to the manycore DPI object
 * @params[in] dpi - Pointer to the manycore DPI object
 */
void SimulationWrapper::dpi_cleanup() {
  delete dpi;
}

/*
 * dpi_fifo_drain
 * Drains all DPI fifos. This function is used during initialization and does not push packets
 * to the FIFOs in Dromajo
 * @param[in] dpi - Pointer to the manycore DPI object
 * @param[in] type - Manycore FIFO type
 * @returns success if all the FIFOs were drained correctly
 */
int SimulationWrapper::dpi_fifo_drain(hb_mc_fifo_rx_t type) {
  __m128i *pkt;

  int err, drains = 0;
  int cap = dpi->config[HB_MC_CONFIG_IO_REMOTE_LOAD_CAP];

  do {
    advance_time();

    switch(type) {
      case HB_MC_FIFO_RX_REQ:
        err = dpi->rx_req(*pkt);
      break;
      case HB_MC_FIFO_RX_RSP:
        err = dpi->rx_rsp(*pkt);
      break;
      default:
        bsg_pr_err("%s: Unknown FIFO type\n", __func__);
        return HB_MC_FAIL;
    }

    if (err == BSG_NONSYNTH_DPI_SUCCESS)
      drains++;
  } while ((err == BSG_NONSYNTH_DPI_NOT_WINDOW || err == BSG_NONSYNTH_DPI_SUCCESS)
          && drains <= cap);

  if (drains == cap) {
    bsg_pr_err("%s: Failed to drain FIFO\n", __func__);
    return HB_MC_FAIL;
  }

  return HB_MC_SUCCESS;
}

/*
 * dpi_init
 * Initializes the DPI between the manycore and Dromajo
 * @params[in] dpi - Pointer to the manycore DPI object
 * @params[in] hierarchy - A C++ string that holds the manycore hierarchy
 * @returns success if initialized correctly
 */
int SimulationWrapper::dpi_init() {
  advance_time();

  int err;

  err = dpi_fifo_drain(HB_MC_FIFO_RX_REQ);
  if (err != HB_MC_SUCCESS) {
    dpi_cleanup();
    return err;
  }

  err = dpi_fifo_drain(HB_MC_FIFO_RX_RSP);
  if (err != HB_MC_SUCCESS) {
    dpi_cleanup();
    return err;
  }

  return HB_MC_SUCCESS;
}

/*
 * dromajo_init
 * Initializes dromajo with the correct command-line arguments
 * @returns a pointer to a dromajo_cosim_state_t object
 */
int SimulationWrapper::dromajo_init() {
  char dromajo_str[50];
  char host_str[50];
  char manycore_str[50];
  char prog_str[50];

  sprintf(dromajo_str, "dromajo");
  sprintf(host_str, "--host");
  sprintf(manycore_str, "--manycore");
  sprintf(prog_str, "main.elf");

  char* argv[] = {dromajo_str, host_str, manycore_str, prog_str};
  dromajo = dromajo_cosim_init(4, argv);
  if (!dromajo) {
    bsg_pr_err("%s: Failed to initialize Dromajo instance!\n", __func__);
    return HB_MC_FAIL;
  }
  return HB_MC_SUCCESS;
}

/*
 * dromajo_step
 * Executes 1 instruction in Dromajo
 * @returns true if execution is incomplete and/or without errors
 */
bool SimulationWrapper::dromajo_step() {
  int err = dromajo_cosim_step(dromajo, 0, 0, 0, 0, 0, false, false);
  return (err != 0) ? false : true;
}

/*
 * dromajo_transmit_packet
 * Fetches data from the Dromajo->Manycore FIFO and if its a host packet, performs
 * the required host operations and pushes responses back on the response FIFO
 * @returns success on succesful transmission
 */
int SimulationWrapper::dromajo_transmit_host_packet(hb_mc_packet_t *dromajo_to_host_packet) {
  hb_mc_packet_t host_to_dromajo_packet;
  int err;

  host_to_dromajo_packet.response.x_dst = dromajo_to_host_packet->request.x_src;
  host_to_dromajo_packet.response.y_dst = dromajo_to_host_packet->request.y_src;
  host_to_dromajo_packet.response.load_id = 0;

  uint32_t data = 0;
  switch (dromajo_to_host_packet->request.op_v2) {
    case HB_MC_PACKET_OP_REMOTE_LOAD:
    {
      // All responses from the host are considered to be of type e_return_int_wb. But a struct similar to
      // bsg_manycore_return_packet_type_e is not defined, so return the source packet's op type. Currently,
      // the response op type is not used for any operation in the platform, therefore this should be OK.
      host_to_dromajo_packet.response.op = dromajo_to_host_packet->request.op_v2;
      switch (dromajo_to_host_packet->request.addr) {
        case HB_BP_HOST_EPA_ARGS_START ... HB_BP_HOST_EPA_ARGS_FINISH:
        {
          // Argument indexes for x86 should be byte addressable but the hardware uses word addresses
          uint32_t arg_index = ((dromajo_to_host_packet->request.addr - HB_BP_HOST_EPA_ARGS_START) >> 2);
          int num_characters = 0;
          // If all arguments have been read or there are no arguments to read
          // send a finish code
          if ((arg_index != argc) && (argc != 0)) {
            // Copy 4 bytes of the arguments
            for(int i = 0; i < 4; i++) {
              if (argv[arg_index][char_index] != '\0') {
                data = (data << 8) | argv[arg_index][char_index];
                num_characters++;
                char_index++;
              }
              else {
                data = (data << (4 - num_characters) * 8);
                char_index = 0;
              }
            }
          }
          else
            data = HB_BP_HOST_OP_FINISH_CODE;
        }
        break;
        case HB_BP_HOST_EPA_CONFIG_START ... HB_BP_HOST_EPA_CONFIG_FINISH:
        {
          uint32_t idx = ((dromajo_to_host_packet->request.addr - HB_BP_HOST_EPA_CONFIG_START) >> 2);
          if (idx <= HB_MC_CONFIG_MAX)
            data = dpi->config[idx];
          else {
            bsg_pr_err("%s: Configuration ROM index out of bounds\n", __func__);
            return HB_MC_NOTFOUND;
          }
        }
        break;
        case HB_BP_HOST_EPA_RESET_DONE:
        {
          bool done;
          dpi->reset_is_done(done);
          data = done ? 1 : 0;
        }
        break;
        case HB_BP_HOST_EPA_TX_VACANT:
        {
          bool is_vacant;
          dpi->tx_is_vacant(is_vacant);
          data = is_vacant ? 1 : 0;
        }
        break;
        default:
        {
          bsg_pr_err("%s: Invalid address for host load operation\n", __func__);
          return HB_MC_FAIL;
        }

      }
      host_to_dromajo_packet.response.data = data;
      for(int j = 0; j < 4; j++) {
        mc_to_host_resp_fifo->fifo[j].push(host_to_dromajo_packet.words[j]);
        host_to_mc_req_fifo->fifo[j].pop();
      }
    }
    break;
    case HB_MC_PACKET_OP_REMOTE_SW:
    {
      switch (dromajo_to_host_packet->request.addr) {
        case HB_MC_HOST_EPA_STDOUT:
        case HB_MC_HOST_EPA_STDERR:
        {
          printf("%c", (uint8_t) dromajo_to_host_packet->request.payload);
          fflush(stdout);
        }
        break;
        case HB_MC_HOST_EPA_FINISH:
        {
          int16_t core_id = dromajo_to_host_packet->request.payload >> 16;
          // Success error codes in BP is 0, but 0 is already used by the manycore. Any positive number indicates
          // success, therefore, add 1.
          int err = (0x0000FFFF & dromajo_to_host_packet->request.payload) + 1;
          bsg_pr_info("Core %d successfully terminated\n", core_id);
          // De-allocate all pointers prior to termination
          dpi_cleanup();
          dromajo_cosim_fini(dromajo);
          return err;
        }
        break;
        case HB_MC_HOST_EPA_FAIL:
        {
          int16_t core_id = dromajo_to_host_packet->request.payload >> 16;
          int16_t err = 0x0000FFFF & dromajo_to_host_packet->request.payload;
          bsg_pr_err("Core %d terminated with code %d\n", core_id, err);
          // De-allocate all pointers prior to termination
          dpi_cleanup();
          dromajo_cosim_fini(dromajo);
          return err;
        }
        break;
        default:
        {
          bsg_pr_err("%s: Invalid address for host store operation\n", __func__);
          return HB_MC_FAIL;
        }
      }
      // Store requests don't have responses so only pop the host to MC fifo
      for(int j = 0; j < 4; j++) {
        host_to_mc_req_fifo->fifo[j].pop();
      }
    }
    break;
    default:
    {
      bsg_pr_err("%s: Operations other than loads and store words are not implemented for the host\n", __func__);
      return HB_MC_FAIL;
    }
    break;
  }

  return HB_MC_SUCCESS;
}

/*
 * dromajo_transmit_packet
 * Fetches data from the Dromajo->Manycore FIFO and pushes it into the DPI FIFO
 * to send packets to the manycore or the host
 * @returns success on succesful transmission
 */
int SimulationWrapper::dromajo_transmit_packet() {
  hb_mc_packet_t dromajo_to_mc_packet, host_to_dromajo_packet;
  int err;
  __m128i *pkt;

  mc_fifo_type_t type = FIFO_HOST_TO_MC_REQ;
  bool is_empty;

  do {
    // Read the FIFO head pointer for all 32-bit FIFOs
    dromajo_to_mc_packet.words[0] = host_to_mc_req_fifo->fifo[0].front();
    dromajo_to_mc_packet.words[1] = host_to_mc_req_fifo->fifo[1].front();
    dromajo_to_mc_packet.words[2] = host_to_mc_req_fifo->fifo[2].front();
    dromajo_to_mc_packet.words[3] = host_to_mc_req_fifo->fifo[3].front();

    // Intercept packets that are for the host and generate appropriate responses
    // TODO: Currently, these packets don't go over the network. In the real system (with BP), some or all of these may be required to go
    // over the network. The platform implementation assumes the host packets do not go over the network. In some cases (like fences) this
    // might not be desirable. Make sure to keep the platform and the system host in sync on what is actually being simulated.
    if ((dromajo_to_mc_packet.request.x_dst == HOST_X_COORD) && (dromajo_to_mc_packet.request.y_dst == HOST_Y_COORD)) {
      err = dromajo_transmit_host_packet(&dromajo_to_mc_packet);
      // This serves to return the error from the host function
      // It is also used to communicate the success code for successful program termination but this code is
      // not HB_MC_SUCCESS (maps to 0) but a positive number
      return err;
    }
    else {
      pkt = reinterpret_cast<__m128i *>(&dromajo_to_mc_packet);

      // Allows the DPI interface to track response FIFO capacity
      bool expect_response =
        (dromajo_to_mc_packet.request.op_v2 != HB_MC_PACKET_OP_REMOTE_STORE) &&
        (dromajo_to_mc_packet.request.op_v2 != HB_MC_PACKET_OP_REMOTE_SW) &&
        (dromajo_to_mc_packet.request.op_v2 != HB_MC_PACKET_OP_CACHE_OP);

      // Attempt packet transmission
      // Since we trigger a call to the transmit FIFOs only when all the 32-bit Dromajo
      // FIFOs are full (i.e. a single 128-bit packet is ready), we need to wait until
      // the DPI FIFOs are ready to receive before advancing to the next operation.
      // This can prevent filling up of the FIFOs. However, not doing this can help in
      // identifying situations that might create backpressure in actual hardware and
      // provision for it.
      do {
        advance_time();
        err = dpi->tx_req(*pkt, expect_response);
      } while (err != BSG_NONSYNTH_DPI_SUCCESS  &&
           (err == BSG_NONSYNTH_DPI_NO_CREDITS  ||
            err == BSG_NONSYNTH_DPI_NO_CAPACITY ||
            err == BSG_NONSYNTH_DPI_NOT_WINDOW  ||
            err == BSG_NONSYNTH_DPI_BUSY        ||
            err == BSG_NONSYNTH_DPI_NOT_READY));

      if (err == BSG_NONSYNTH_DPI_SUCCESS) {
        for (int i = 0;i < 4; i++)
          host_to_mc_req_fifo->fifo[i].pop();
      }
      else {
        bsg_pr_err("%s: Packet transmission failed\n", __func__);
        return HB_MC_FAIL;
      }
    }
    is_empty = mc_is_fifo_empty(type);
  } while (!is_empty);

  // Update the credits in dromajo
  err = dromajo_update_credits();
  if (err != HB_MC_SUCCESS)
    return err;

  return HB_MC_SUCCESS;
}

/*
 * dromajo_receive_packet
 * Receives packets from the DPI FIFOs and pushes the data into the
 * manycore request and response FIFOs in Dromajo
 */
int SimulationWrapper::dromajo_receive_packet() {
  hb_mc_packet_t *mc_to_dromajo_req_packet, *mc_to_dromajo_resp_packet;
  int err;
  __m128i pkt;

  // Read from the manycore request FIFO
  // At every clock edge we are polling the request FIFO to see if there
  // is a packet. If there is no packet (i.e err == BSG_NONSYNTH_NOT_VALID)
  // we must move on
  do {
    do {
      advance_time();
      err = dpi->rx_req(pkt);
    } while (err != BSG_NONSYNTH_DPI_SUCCESS    &&
            (err == BSG_NONSYNTH_DPI_NOT_WINDOW ||
             err == BSG_NONSYNTH_DPI_BUSY));

    if (err == BSG_NONSYNTH_DPI_SUCCESS) {
      mc_to_dromajo_req_packet = reinterpret_cast<hb_mc_packet_t *>(&pkt);
      for (int i = 0; i < 4; i++) {
        mc_to_host_req_fifo->fifo[i].push(mc_to_dromajo_req_packet->words[i]);
      }
    }
  } while (err != BSG_NONSYNTH_DPI_NOT_VALID);

  // Read from the manycore response FIFO
  do {
    do {
      advance_time();
      err = dpi->rx_rsp(pkt);
    } while (err != BSG_NONSYNTH_DPI_SUCCESS    &&
            (err == BSG_NONSYNTH_DPI_NOT_WINDOW ||
             err == BSG_NONSYNTH_DPI_BUSY));

    if (err == BSG_NONSYNTH_DPI_SUCCESS) {
      mc_to_dromajo_resp_packet = reinterpret_cast<hb_mc_packet_t *>(&pkt);
      for (int i = 0; i < 4; i++) {
        mc_to_host_resp_fifo->fifo[i].push(mc_to_dromajo_resp_packet->words[i]);
      }
    }
  } while (err != BSG_NONSYNTH_DPI_NOT_VALID);

  return HB_MC_SUCCESS;
}

/*
 * dromajo_update_credits
 * Polls the hardware for credit information and updates the credits info
 * for the Dromajo->Manycore request FIFO in dromajo
 */
int SimulationWrapper::dromajo_update_credits() {
  int credits_used;
  int err;

  err = dpi->get_credits_used(credits_used);
  if (err != BSG_NONSYNTH_DPI_SUCCESS) {
    bsg_pr_err(bsg_nonsynth_dpi_strerror(err));
    return HB_MC_FAIL;
  }

  if (credits_used < 0) {
    bsg_pr_err("%s: Credit value is < 0. Must be non-negative\n", __func__);
    return err;
  }
  host_to_mc_req_fifo->credits = credits_used;

  return HB_MC_SUCCESS;
}

int SimulationWrapper::eval(){
  bool transmit = false;
  int err;

  // Execute NUM_DROMAJO_INSTR_PER_TICK instructions on Dromajo
  for(int i = 0; i < NUM_DROMAJO_INSTR_PER_TICK; i++) {
    if (!dromajo_step()) {
      // Fixme: Dromajo could also terminate due to premature errors.
      // Use the test outputs to detect PASS/FAIL
      bsg_pr_info("Dromajo Execution Complete!\nExiting...\n");
      dromajo_cosim_fini(dromajo);
      return HB_MC_SUCCESS;
    }

    // Advancing time is required for things to move around in hardware, however
    // it adds a large overhead to the simulation time. Balance these values to
    // hit that sweet spot.
    if ((i % 10) == 0)
      advance_time_cycles(10);

    // Check if transmit FIFO has an element and hence ready to transmit
    // This operation is relatively low overhead since only a single FIFO's
    // empty status is being checked. Hence, we can afford to do it every 100
    // iterations.
    if (i % NUM_TX_FIFO_CHK_PER_TICK) {
      mc_fifo_type_t type = FIFO_HOST_TO_MC_REQ;
      bool is_empty = mc_is_fifo_empty(type);
      if (!is_empty) {
        transmit = true;
        break;
      }
    }
  }

  if (transmit) {
    // Poll for packets to be transmitted and transmit them
    bsg_pr_dbg("Checking for packets to transmit\n");
    err = dromajo_transmit_packet();
    if (err != HB_MC_SUCCESS)
      return err;
  }

  // Poll for packets to be received and push them to Dromajo
  // This involves a DPI call and might incur a large overhead, therefore
  // check for packets to receive only once per tick
  bsg_pr_dbg("Checking for packets to receive\n");
  err = dromajo_receive_packet();
  if (err != HB_MC_SUCCESS)
    return err;

  return HB_MC_SUCCESS;
}

////////////////////////////// Interacting with VCS //////////////////////////////

/*
 * get_argc
 * Given a string, determine the number of space-separated arguments
 * @params[in] args - Pointer to a character array that holds the arguments
 * @returns the number of arguments
 */
static int get_argc(char * args){
        char *cur = args, prev=' ';
        int count = 1;
        while(*cur != '\0'){
                if((prev == ' ') && (prev != *cur)){
                        count ++;
                }
                prev = *cur;
                ++cur;
        }
        return count;
}

/*
 * get_argv
 * Given a string, retrieves the space-separated arguments
 * @params[in] args - Pointer to a character array that holds the arguments
 * @params[in] argc - Number of arguments
 * @params[in] argv - Pointer to an array of strings that will hold the different
 * arguments
 */
static void get_argv(char * args, int argc, char **argv){
        int count = 0;
        char *cur = args, prev=' ';

        // First parse the path name. This is not in the argument string because
        // VCS doesn't provide it to us. Instead, we "hack" around it by reading
        // the path from 'proc/self/exe'. The maximum path-name length is 1024,
        // with an extra null character for safety
        static char path[1025] = {'\0'};

        readlink("/proc/self/exe", path, sizeof(path) - 1);
        argv[0] = path;
        count ++;

        // Then we parse the remaining arguments. Arguments are separated by N
        // >= 1 spaces. We only register an argument when the previous character
        // was a space, and the current character is not (so that multiple
        // spaces don't count as multiple arguments). We replace spaces with
        // null characters (\0) so that each argument appears to be an
        // individual string and can be used later, by argparse (and other
        // libraries)
        while(*cur != '\0'){
                if((prev == ' ') && (prev != *cur)){
                        argv[count] = cur;
                        count++;
                }
                prev = *cur;
                if(*cur == ' ')
                        *cur = '\0';
                cur++;
        }
}

/*
 * vcs_main
 * The main function for the x86 host. It initializes all devices and controls
 * the time in simulation. It handles all DPI calls and passes packets to and from
 * Dromajo during execution
 * @params[in] argc - Number of arguments
 * @params[in] argv - Pointer to an array of string arguments
 * @returns
 */
int vcs_main(int argc, char **argv) {
  // Initialize Host
  SimulationWrapper *host = new SimulationWrapper();
  if (!host) {
    bsg_pr_err("%s: Could not initialize host!\n", __func__);
    return HB_MC_FAIL;
  }

  // Assign arguments
  host->set_args(argc, argv);

  int err;
  // TODO: Currently the simulation terminates by encoding the error code and the core
  // ID in the 32-bit data field of a finish packet sent to the host. Need to come up
  // with a more elegant solution for this.
  do {
    err = host->eval();
    // Codes greater than 0 can be used to terminate a program
    if (err > 0) break;
  } while (err >= 0);

  return err;
}

/*
 * cosim_main
 * This function is the VCS hook for cosimulation
 * @params[in] exit_code - A pointer to an integer that holds the exit code
 * @params[in] args - A character array that holds the space-separated
 * arguments to the function
 */
extern "C" {
  void cosim_main(uint32_t *exit_code, char *args) {
          // We aren't passed command line arguments directly so we parse them
          // from *args. args is a string from VCS - to pass a string of arguments
          // to args, pass c_args to VCS as follows: +c_args="<space separated
          // list of args>"
          int argc = get_argc(args);
          char *argv[argc];
          get_argv(args, argc, argv);

          int rc = vcs_main(argc, argv);
          *exit_code = rc;
          bsg_pr_test_pass_fail(rc >= HB_MC_SUCCESS);
          return;
  }
}
