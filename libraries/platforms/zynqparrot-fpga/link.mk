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

# This Makefile Fragment defines rules for linking object files for native
# regression tests

# hardware.mk is the file list for the simulation RTL. It includes the
# platform specific hardware.mk file.
include $(HARDWARE_PATH)/hardware.mk

# libraries.mk defines how to build libbsg_manycore_runtime.so, which is
# pre-linked against all other simulation binaries.
include $(LIBRARIES_PATH)/libraries.mk

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

LDFLAGS += -L$(BSG_PLATFORM_PATH)
LDFLAGS += -lbsg_manycore_runtime -lbsg_manycore_regression -lbsgmc_cuda_legacy_pod_repl -lm

TEST_CSOURCES   += $(filter %.c,$(TEST_SOURCES))
TEST_CXXSOURCES += $(filter %.cpp,$(TEST_SOURCES))
TEST_OBJECTS    += $(TEST_CXXSOURCES:.cpp=.o)
TEST_OBJECTS    += $(TEST_CSOURCES:.c=.o)

REGRESSION_LIBRARIES += $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so
REGRESSION_LIBRARIES += $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
REGRESSION_LIBRARIES += $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so

loader.o: $(TEST_OBJECTS) $(REGRESSION_LIBRARIES)
	$(CXX) -o $@ $^ $(LDFLAGS) $(CDEFINES)

.PHONY: platform.link.clean
platform.link.clean:
	rm -rf loader.o

link.clean: platform.link.clean

