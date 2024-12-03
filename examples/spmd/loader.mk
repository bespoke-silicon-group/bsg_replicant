# Copyright (c) 2021, University of Washington All rights reserved.
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

################################################################################
# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# LIBRAIRES_PATH: The path to the libraries directory
# HARDWARE_PATH: The path to the hardware directory
# EXAMPLES_PATH: The path to the examples directory
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
###############################################################################

REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk
SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd

ifndef SPMD_NAME
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: SPMD_NAME is not defined$(NC)"))
endif

###############################################################################
# Host code compilation flags and flow
###############################################################################
TILE_GROUP_DIM_X ?= 1
TILE_GROUP_DIM_Y ?= 1
LINK_GEN_MOVE_RODATA_TO_DMEM ?= 0

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = $(CURDIR)/loader.c
$(CURDIR)/loader.c: $(EXAMPLES_PATH)/spmd/loader.c
	ln -s $^ $@

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -D_DEFAULT_SOURCE
CDEFINES += 
CXXDEFINES += 

# Specify number of pods to launch, default all available pods (-1)
HB_MC_POD_GROUP_X ?= -1
HB_MC_POD_GROUP_Y ?= -1
CDEFINES   += -DHB_MC_POD_GROUP_X=$(HB_MC_POD_GROUP_X) -DHB_MC_POD_GROUP_Y=$(HB_MC_POD_GROUP_Y)
CXXDEFINES += -DHB_MC_POD_GROUP_X=$(HB_MC_POD_GROUP_X) -DHB_MC_POD_GROUP_Y=$(HB_MC_POD_GROUP_Y)

# Specify if waiting for finish packets from all tiles, default no (0)
HB_MC_WAIT_ALL_TILES_DONE ?= 0
CDEFINES   += -DHB_MC_WAIT_ALL_TILES_DONE=$(HB_MC_WAIT_ALL_TILES_DONE)
CXXDEFINES += -DHB_MC_WAIT_ALL_TILES_DONE=$(HB_MC_WAIT_ALL_TILES_DONE)

# Specify if launching multiple pods in series, default no (0)
HB_MC_LAUNCH_PODS_IN_SERIES ?= 0
CDEFINES   += -DHB_MC_LAUNCH_PODS_IN_SERIES=$(HB_MC_LAUNCH_PODS_IN_SERIES)
CXXDEFINES += -DHB_MC_LAUNCH_PODS_IN_SERIES=$(HB_MC_LAUNCH_PODS_IN_SERIES)

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++11 $(FLAGS)

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

###############################################################################
# Host code link flags and flow
###############################################################################

# link.mk defines rules for linking of the final execution binary.
include $(EXAMPLES_PATH)/link.mk

###############################################################################
# Device code compilation flow
###############################################################################

# BSG_MANYCORE_KERNELS is a list of manycore executables that should
# be built before executing.
BSG_MANYCORE_KERNELS = $(SPMD_SRC_PATH)/$(SPMD_NAME)/main.riscv

$(SPMD_SRC_PATH)/$(SPMD_NAME)/main.riscv:
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	bsg_tiles_X=$(TILE_GROUP_DIM_X) \
	bsg_tiles_Y=$(TILE_GROUP_DIM_Y) \
	LINK_GEN_MOVE_RODATA_TO_DMEM=$(LINK_GEN_MOVE_RODATA_TO_DMEM) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -j1 -C $(SPMD_SRC_PATH)/$(SPMD_NAME) main.riscv

###############################################################################
# Execution flow
#
# C_ARGS: Use this to pass arguments that you want to appear in argv
#         For SPMD tests C arguments are: <Path to RISC-V Binary> <Test Name>
#
# SIM_ARGS: Use this to pass arguments to the simulator
###############################################################################
C_ARGS ?= $(BSG_MANYCORE_KERNELS) $(SPMD_NAME) $(TILE_GROUP_DIM_X) $(TILE_GROUP_DIM_Y)

SIM_ARGS ?=

# Include platform-specific execution rules
include $(EXAMPLES_PATH)/execution.mk

###############################################################################
# Regression Flow
###############################################################################

regression: exec.log
	@grep "BSG REGRESSION TEST .*PASSED.*" $< > /dev/null

.DEFAULT_GOAL := help

.PHONY: clean

clean:
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -j1 -C $(SPMD_SRC_PATH)/$(SPMD_NAME) clean
	rm -f $(CURDIR)/loader.c


