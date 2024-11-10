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

PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/features/profiler/noimpl/bsg_manycore_profiler.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/features/tracer/noimpl/bsg_manycore_tracer.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/features/dma/blackparrot/bsg_manycore_dma.cpp
PLATFORM_CXXSOURCES += $(BSG_PLATFORM_PATH)/bsg_manycore_platform.cpp

LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-ba.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-eexst.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-fmtstream.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-fs-xinl.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-help.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-parse.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-pv.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-pvh.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/argp/argp-xinl.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/flockfile.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/src/funlockfile.c

include $(LIBRARIES_PATH)/features/dma/blackparrot/feature.mk

PLATFORM_OBJECTS += $(patsubst %cpp,%o,$(PLATFORM_CXXSOURCES))
PLATFORM_OBJECTS += $(patsubst %c,%o,$(PLATFORM_CSOURCES))

PLATFORM_REGRESSION_OBJECTS += $(patsubst %cpp,%o,$(PLATFORM_REGRESSION_CXXSOURCES))
PLATFORM_REGRESSION_OBJECTS += $(patsubst %c,%o,$(PLATFORM_REGRESSION_CSOURCES))

$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES := -I$(LIBRARIES_PATH)
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(BSG_PLATFORM_PATH)
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(BSG_PLATFORM_PATH)/include
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/dma
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/profiler
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/tracer
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(ZYNQPARROT_DIR)/cosim/include/common
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(ZYNQPARROT_DIR)/cosim/include/fpga
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): $(BSG_MACHINE_PATH)/bsg_manycore_machine.h
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(BSG_MACHINE_PATH)

$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(CL_DIR)/../aws-fpga/sdk/userspace/include
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): CFLAGS   := -O3 -std=c11 -fPIC -D_BSD_SOURCE -D_GNU_SOURCE -D_DEFAULT_SOURCE $(INCLUDES)
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): CXXFLAGS := -O3 -std=c++11 -fPIC -D_BSD_SOURCE -D_GNU_SOURCE -D_DEFAULT_SOURCE $(INCLUDES)
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): LDFLAGS   = -fPIC
$(PLATFORM_REGRESSION_OBJECTS): LDFLAGS   = -ldl

$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.a: $(PLATFORM_OBJECTS)
$(BSG_PLATFORM_PATH)/libbsg_manycore_regression.a: $(PLATFORM_REGRESSION_OBJECTS)

.PHONY: platform.clean install uninstall
platform.clean:
	rm -f $(PLATFORM_OBJECTS)
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so*
	rm -f $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so*

libraries.clean: platform.clean

