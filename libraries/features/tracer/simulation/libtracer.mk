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

# This Makefile fragment is for building the simulation tracer runtime library
# library for cosimulation
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
#

# LIBRARIES_PATH: The path to the libraries folder in F1
ifndef LIBRARIES_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: LIBRARIES_PATH is not defined$(NC)"))
endif

ifndef BASEJUMP_STL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BASEJUMP_STL_DIR is not defined$(NC)"))
endif

# Don't include more than once
ifndef (_BSG_F1_TESTBENCHES_LIBTRACER_MK)
_BSG_F1_TESTBENCHES_LIBTRACER_MK := 1

$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS += -L$(LIBRARIES_PATH)/features/tracer/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/tracer/simulation -ltracer
$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: $(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so

# Add a clean rule
.PHONY: tracer.clean
tracer.clean:
	rm -f $(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so

# Add as a subrule to simlibs.clean
libraries.clean: tracer.clean

# Rules for building Simulation tracer runtime library
$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: CXXFLAGS += -std=c++11 -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE -Wall -fPIC -shared
$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: CXXFLAGS += -I$(BSG_MANYCORE_DIR)/testbenches/common/v
# Uncomment to enable Verilator profiling with operf
#$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: CXXFLAGS += -g -pg
$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: CXX=g++
$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_profiler.cpp
$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_profiler.cpp
$(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so: $(BSG_MANYCORE_DIR)/testbenches/common/v/remote_load_profiler.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -Wl,-soname,$(notdir $@) -o $@

endif # ifndef (_BSG_F1_TESTBENCHES_LIBTRACER_MK)
