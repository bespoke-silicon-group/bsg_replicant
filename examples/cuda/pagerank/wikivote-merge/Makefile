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
GRAPHIT_PATH:=$(EXAMPLES_PATH)/cuda/pagerank/graphit

# Run `make checkout_graphit`
.PHONY: checkout_graphit
checkout_graphit:
	cd ..
	git clone git@github.com:bespoke-silicon-group/graphit

###############################################################################
# Host code compilation flags and flow
###############################################################################

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = pr_merge.cpp

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -D_DEFAULT_SOURCE
CDEFINES += 
CXXDEFINES += -DSIM_CURRENT_POD=1

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
FLAGS	 += -I$(GRAPHIT_PATH)/src/runtime_lib
FLAGS	 += -O2
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++14 $(FLAGS)

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

###############################################################################
# Host code link flags and flow
###############################################################################

LDFLAGS +=

# link.mk defines rules for linking of the final execution binary.
include $(EXAMPLES_PATH)/link.mk

###############################################################################
# Device code compilation flow
###############################################################################

# BSG_MANYCORE_KERNELS is a list of manycore executables that should
# be built before executing.
BSG_MANYCORE_KERNELS = kernel.riscv

vpath %.cpp $(EXAMPLES_PATH)/cuda/pagerank/kernel/pr_merge

kernel.rvo:RISCV_CXX=$(RISCV_CLANGXX)
kernel.riscv: kernel.rvo

# Tile Group Dimensions
TILE_GROUP_DIM_X = $(BSG_MACHINE_GLOBAL_X)
TILE_GROUP_DIM_Y = $(BSG_MACHINE_GLOBAL_Y)
RISCV_DEFINES += -Dbsg_tiles_X=$(TILE_GROUP_DIM_X)
RISCV_DEFINES += -Dbsg_tiles_Y=$(TILE_GROUP_DIM_Y)
RISCV_DEFINES += -DSIM_KERNEL_CURRENT_POD=1
RISCV_DEFINES += -DSIM_KERNEL_CURRENT_BLOCK=1
RISCV_CCPPFLAGS += -O2
RISCV_CCPPFLAGS += -I$(GRAPHIT_PATH)/src/runtime_lib/infra_hb/device
RISCV_CXXFLAGS += -I$(EXAMPLES_PATH)/cuda/pagerank/kernel/include

include $(EXAMPLES_PATH)/cuda/riscv.mk

###############################################################################
# Execution flow
#
# C_ARGS: Use this to pass arguments that you want to appear in argv
#         For SPMD tests C arguments are: <Path to RISC-V Binary> <Test Name>
#
# SIM_ARGS: Use this to pass arguments to the simulator
###############################################################################
C_ARGS ?= $(BSG_MANYCORE_KERNELS) -g /home/zz546/gapbs/suitsparse_wikivote.el

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


