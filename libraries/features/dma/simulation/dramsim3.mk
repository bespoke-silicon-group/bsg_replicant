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

# This Makefile fragment is for building the dramsim3 library for
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

# LIBRARIES_PATH: The path to the regression folder in BSG F1
ifndef LIBRARIES_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: LIBRARIES_PATH is not defined$(NC)"))
endif

# Keep execution, profiling and tracing libraries independent: a library with one
# SONAME cannot safely change instrumentation when another mode is built.
ifndef _BSG_F1_TESTBENCHES_DRAMSIM3_MK
_BSG_F1_TESTBENCHES_DRAMSIM3_MK := 1
DRAMSIM3_MAKEFILE := $(lastword $(MAKEFILE_LIST))
DRAMSIM3_BUILD_HELPER := $(dir $(DRAMSIM3_MAKEFILE))dramsim3_build.py
DRAMSIM3_OPT_FLAGS ?= -O2
PYTHON ?= python3
DRAMSIM3_PATH := $(LIBRARIES_PATH)/features/dma/simulation
DRAMSIM3_SOURCE_PATH := $(BASEJUMP_STL_DIR)/imports/DRAMSim3
DRAMSIM3_LEGACY_LIBRARY := $(DRAMSIM3_PATH)/libdramsim3.so
DRAMSIM3_EXEC_LIBRARY := $(DRAMSIM3_PATH)/libdramsim3_exec.so
DRAMSIM3_PROFILE_LIBRARY := $(DRAMSIM3_PATH)/libdramsim3_profile.so
DRAMSIM3_TRACE_LIBRARY := $(DRAMSIM3_PATH)/libdramsim3_trace.so
DRAMSIM3_LIBRARIES := $(DRAMSIM3_LEGACY_LIBRARY) $(DRAMSIM3_EXEC_LIBRARY) $(DRAMSIM3_PROFILE_LIBRARY) $(DRAMSIM3_TRACE_LIBRARY)
DRAMSIM3_CONFIGS := $(addsuffix .config,$(DRAMSIM3_LIBRARIES))

ifneq ($(filter hbm2, $(subst _, ,$(BSG_MACHINE_MEM_CFG))),)

# Verilator selects its DRAM library at the final executable link. The common
# runtime must not pull the profiling library into an exec process indirectly.
ifeq ($(BSG_PLATFORM),bigblade-verilator)
$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: $(DRAMSIM3_MAKEFILE)
else
$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS += -L$(DRAMSIM3_PATH) $(call RPATH,$(DRAMSIM3_PATH)) -ldramsim3
$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: $(DRAMSIM3_LEGACY_LIBRARY)
endif

DRAMSIM3_SOURCES := $(addprefix $(DRAMSIM3_SOURCE_PATH)/src/,bankstate.cc channel_state.cc command_queue.cc common.cc configuration.cc controller.cc dram_system.cc hmc.cc memory_system.cc refresh.cc simple_stats.cc timing.cc)
DRAMSIM3_SOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_dramsim3.cpp
DRAMSIM3_HEADERS := $(wildcard $(DRAMSIM3_SOURCE_PATH)/src/*.h $(DRAMSIM3_SOURCE_PATH)/ext/headers/*.h $(DRAMSIM3_SOURCE_PATH)/ext/headers/*.hpp $(DRAMSIM3_SOURCE_PATH)/ext/fmt/include/fmt/*.h)

$(DRAMSIM3_LIBRARIES) $(DRAMSIM3_CONFIGS): private INCLUDES :=
$(DRAMSIM3_LIBRARIES) $(DRAMSIM3_CONFIGS): private CXXFLAGS = $(DRAMSIM3_OPT_FLAGS) -std=c++11 -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE -Wall -fPIC \
  -I$(DRAMSIM3_SOURCE_PATH)/src -I$(DRAMSIM3_SOURCE_PATH)/ext/headers -I$(DRAMSIM3_SOURCE_PATH)/ext/fmt/include \
  -DFMT_HEADER_ONLY=1 -DBASEJUMP_STL_DIR="$(BASEJUMP_STL_DIR)" $(DRAMSIM3_DEFINES)
$(DRAMSIM3_LEGACY_LIBRARY) $(DRAMSIM3_LEGACY_LIBRARY).config: DRAMSIM3_DEFINES := -DBLOOD_GRAPH
$(DRAMSIM3_EXEC_LIBRARY) $(DRAMSIM3_EXEC_LIBRARY).config: DRAMSIM3_DEFINES := -DDRAMSIM3_NO_STATISTICS
$(DRAMSIM3_PROFILE_LIBRARY) $(DRAMSIM3_PROFILE_LIBRARY).config: DRAMSIM3_DEFINES :=
$(DRAMSIM3_TRACE_LIBRARY) $(DRAMSIM3_TRACE_LIBRARY).config: DRAMSIM3_DEFINES := -DBLOOD_GRAPH -DBLOOD_GRAPH_ENABLE_TRACE
$(DRAMSIM3_LEGACY_LIBRARY) $(DRAMSIM3_TRACE_LIBRARY): $(DRAMSIM3_SOURCE_PATH)/src/blood_graph.cc

.PHONY: dramsim3-config-force
dramsim3-config-force:

# Preserve the stamp on identical builds; compiler/options changes invalidate
# only the affected library. The helper also checks the required source API.
$(DRAMSIM3_CONFIGS): dramsim3-config-force
	@$(PYTHON) $(DRAMSIM3_BUILD_HELPER) "$@" --compiler '$(CXX)' --flags '$(CXXFLAGS)' \
	  --link-flags '$(SHARED_LIBRARY_FLAGS) $(call SHARED_LIBRARY_ID,$(notdir $(basename $@)))' \
	  --source '$(DRAMSIM3_SOURCE_PATH)'

$(DRAMSIM3_LIBRARIES): %: %.config
$(DRAMSIM3_LIBRARIES): $(DRAMSIM3_SOURCES) $(DRAMSIM3_HEADERS) $(DRAMSIM3_MAKEFILE) $(DRAMSIM3_BUILD_HELPER)
	$(CXX) $(CXXFLAGS) $(filter %.cc %.cpp,$^) $(SHARED_LIBRARY_FLAGS) $(call SHARED_LIBRARY_ID,$(notdir $@)) -o $@

endif # hbm2
.PHONY: dramsim3.clean
dramsim3.clean:
	rm -f $(DRAMSIM3_LIBRARIES) $(DRAMSIM3_CONFIGS)
libraries.clean: dramsim3.clean
endif
