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

# This Makefile compiles, links, and executes examples Run `make help`
# to see the available targets for the selected platform.

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# LIBRAIRES_PATH: The path to the libraries directory
# HARDWARE_PATH: The path to the hardware directory
# EXAMPLES_PATH: The path to the examples directory
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../../environment.mk

REGRESSION_TESTS_TYPE = spmd

###############################################################################
# Examples List
#
# UNIFIED_TESTS use a generic top-level, test_unified_main.c. They depend on a
# self-checking Manycore kernel to send a PASS/FAIL packet to the host
#
# INDEPENDENT_TESTS use a per-test top level, <test_name>.c/cpp. They check
# correctness on the host.
# 
# REGRESSION_TESTS merges the two lists
###############################################################################
UNIFIED_TESTS = test_fib 
UNIFIED_TESTS += test_bsg_print_stat
UNIFIED_TESTS += test_putchar_stream

INDEPENDENT_TESTS := test_bsg_dram_loopback_cache
INDEPENDENT_TESTS += test_symbol_to_eva
INDEPENDENT_TESTS += test_bsg_loader_suite
INDEPENDENT_TESTS += test_bsg_scalar_print

REGRESSION_TESTS = $(UNIFIED_TESTS) $(INDEPENDENT_TESTS)

###############################################################################
# Host code compilation flags and flow
###############################################################################
# The following define is DEPRECATED. Do not use BSG_MANYCORE_DIR as a
# macro! Instead, pass it as an argument and parse it using argparse.
DEFINES += -DBSG_MANYCORE_DIR=$(abspath $(BSG_MANYCORE_DIR))

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

CDEFINES   += $(DEFINES)
CXXDEFINES += $(DEFINES)

FLAGS     = -g -Wall
CFLAGS   += -std=c11 $(FLAGS) 
CXXFLAGS += -std=c++11 $(FLAGS)

###############################################################################
# Execution Arguments (C_ARGS)
#
# All SPMD programs require two arguments: The path to the RISC-V Binary loaded
# onto the manycore (KERNEL_PATH), and the test name (TEST_NAME).
#
###############################################################################

SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd
KERNEL_PATH = $(SPMD_SRC_PATH)/$(TEST_NAME:test_%=%)/main.riscv
C_ARGS = $(KERNEL_PATH) $(TEST_NAME)

include $(EXAMPLES_PATH)/flow.mk

###############################################################################
# Kernel Binary Build Rules
###############################################################################

# Force rebuild targets that depend on .FORCE, like the
# Manycore/SPMD/CUDA Binaries
.FORCE:

.PHONY: test_%.clean test_%.rule

$(USER_RULES): test_%.rule: $(SPMD_SRC_PATH)/%/main.riscv

$(USER_CLEAN_RULES): 
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -j1 -C $(SPMD_SRC_PATH)/$(subst .clean,,$(subst test_,,$@)) clean

$(SPMD_SRC_PATH)/%/main.riscv: $(BSG_MACHINE_PATH)/Makefile.machine.include .FORCE
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	bsg_tiles_X=$(TILE_GROUP_DIM_X) \
	bsg_tiles_Y=$(TILE_GROUP_DIM_Y) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -j1 -C $(dir $@) clean $(notdir $@)

.PHONY: clean

clean:
	rm -rf $(INDEPENDENT_TESTS) test_loader
