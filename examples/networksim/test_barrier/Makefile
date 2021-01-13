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

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# LIBRAIRES_PATH: The path to the libraries directory
# HARDWARE_PATH: The path to the hardware directory
# EXAMPLES_PATH: The path to the examples directory
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk

# Set TEST_NAME to the name of this directory. For this test, it
# matches the name of the source file but that is not required. If the
# .cpp file has a different name that the directory you can manually
# type it instead.
TEST_NAME = $(notdir $(shell pwd))

###############################################################################
# Host code compilation flags and flow
###############################################################################

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = $(TEST_NAME).cpp

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++11 $(FLAGS)

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

# Again, TEST_NAME is not required
$(TEST_NAME).o: INCLUDES += -I$(EXAMPLES_PATH)
$(TEST_NAME).o: INCLUDES += -I$(BSG_F1_DIR)/examples/networksim
$(TEST_NAME).o: $(BSG_MACHINE_PATH)/bsg_manycore_machine.h
$(TEST_NAME).o: $(BSG_F1_DIR)/examples/networksim/bsg_manycore_dpi_tile.hpp

router_utilization.rpt: $(TEST_NAME).log
	python3 ../router_parser.py

.PHONY: router_utilization.rpt

###############################################################################
# Host code link flags and flow
###############################################################################

LDFLAGS += -lbsgmc_cuda_legacy_pod_repl 

# link.mk defines rules for linking of the final execution binary.
include $(EXAMPLES_PATH)/link.mk


###############################################################################
# C_ARGS: Use this to pass arguments that you want to appear in argc/argv
#
# SIM_ARGS: Use this to pass arguments to the simulator
#
###############################################################################
C_ARGS ?=

SIM_ARGS ?=

# Include platform-specific execution rules
include $(EXAMPLES_PATH)/execution.mk

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {clean|<test_name>|<test_name>.log}"
	@echo "      $(TEST_NAME): Build executable for this platform"
	@echo "      $(TEST_NAME).log: Run test and generate the log file"
	@echo "      $(TEST_NAME).vpd: Build waveform executable (if VCS)"
	@echo "      clean: Remove all subdirectory-specific outputs"

.PHONY: clean

clean:
	rm -rf $(TEST_NAME)
