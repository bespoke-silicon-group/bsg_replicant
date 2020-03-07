# Copyright (c) 2019, University of Washington All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This Makefile fragment is for building the ramulator library for
# cosimulation
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
#

# CL_DIR: The path to the root of the BSG F1 Repository
ifndef CL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: CL_DIR is not defined$(NC)"))
endif

# TESTBENCH_PATH: The path to the testbenches folder in BSG F1
ifndef TESTBENCH_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: TESTBENCH_PATH is not defined$(NC)"))
endif

# LIBRARIES_PATH: The path to the regression folder in BSG F1
ifndef LIBRARIES_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: LIBRARIES_PATH is not defined$(NC)"))
endif

# PROJECT: The project name, used to as the work directory of the hardware
# library during analysis
ifndef PROJECT
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: PROJECT is not defined$(NC)"))
endif

# Don't include more than once
ifndef (_BSG_F1_TESTBENCHES_DRAMSIM3_MK)
_BSG_F1_TESTBENCHES_DRAMSIM3_MK := 1
_DRAMSIM3_MEM_CFGS := e_vcache_non_blocking_dramsim3_hbm2_4gb_x128
_DRAMSIM3_MEM_CFGS += e_vcache_blocking_dramsim3_hbm2_4gb_x128
# Check if ramulator is the memory model for this design
ifneq ($(filter $(_DRAMSIM3_MEM_CFGS), $(CL_MANYCORE_MEM_CFG)),)

# Disable the micron memory model (it's unused and slows simulation WAY down)
VDEFINES   += AXI_MEMORY_MODEL=1
VDEFINES   += ECC_DIRECT_EN
VDEFINES   += RND_ECC_EN
VDEFINES   += ECC_ADDR_LO=0
VDEFINES   += ECC_ADDR_HI=0
VDEFINES   += RND_ECC_WEIGHT=0

# Flag that DRAMSim3 is being used
VDEFINES   += USING_DRAMSIM3=1

# Define the name and package of the memory technology
DRAMSIM3_MEMORY:=$(patsubst e_vcache_blocking_dramsim3_%, %, \
	$(filter e_vcache_blocking_%, $(CL_MANYCORE_MEM_CFG)))

DRAMSIM3_MEMORY+=$(patsubst e_vcache_non_blocking_dramsim3_%, %, \
	$(filter e_vcache_non_blocking_%, $(CL_MANYCORE_MEM_CFG)))

DRAMSIM3_MEMORY:=$(strip $(DRAMSIM3_MEMORY))
DRAMSIM3_MEM_PKG:=bsg_dramsim3_$(DRAMSIM3_MEMORY)_pkg

VDEFINES   += DRAMSIM3_MEMORY=$(DRAMSIM3_MEMORY)
VDEFINES   += DRAMSIM3_MEM_PKG=$(DRAMSIM3_MEM_PKG)

# Add DRAMSim3 to the simlibs
SIMLIBS += $(TESTBENCH_PATH)/libdramsim3.so
LDFLAGS += -L$(TESTBENCH_PATH) -Wl,-rpath=$(TESTBENCH_PATH) -ldramsim3

# Define USING_DRAMSIM3 for host library
$(LIB_OBJECTS): CXXFLAGS += -DUSING_DRAMSIM3=1

# Add a clean rule
.PHONY: dramsim3.clean
dramsim3.clean:
	rm -f $(TESTBENCH_PATH)/libdramsim3.so

# Add as a subrule to simlibs.clean
simlibs.clean: dramsim3.clean

# Rules for building ramulator library
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -std=c++11 -D_GNU_SOURCE -Wall -fPIC -shared
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -I$(BASEJUMP_STL_DIR)/imports/DRAMSim3/src
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -I$(BASEJUMP_STL_DIR)/imports/DRAMSim3/ext/headers
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -I$(BASEJUMP_STL_DIR)/imports/DRAMSim3/ext/fmt/include
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -I$(BASEJUMP_STL_DIR)/bsg_test
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -DFMT_HEADER_ONLY=1
$(TESTBENCH_PATH)/libdramsim3.so: CXXFLAGS += -DBASEJUMP_STL_DIR="$(BASEJUMP_STL_DIR)"
$(TESTBENCH_PATH)/libdramsim3.so: CXX=g++

$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/bankstate.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/channel_state.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/command_queue.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/common.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/configuration.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/controller.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/dram_system.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/hmc.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/memory_system.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/refresh.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/simple_stats.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/imports/DRAMSim3/src/timing.cc
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/bsg_test/bsg_dramsim3.cpp
$(TESTBENCH_PATH)/libdramsim3.so: $(BASEJUMP_STL_DIR)/bsg_test/bsg_test_dram_channel.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -Wl,-soname,$(notdir $@) -o $@

endif # ifneq ($(filter $(_DRAMSIM3_MEM_CFGS), $(CL_MANYCORE_MEM_CFG)),)
endif # ifndef(_BSG_F1_TESTBENCHES_DRAMSIM3_MK)
