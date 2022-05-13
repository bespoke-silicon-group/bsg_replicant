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


# KERNEL_NAME is the name of the CUDA-Lite Kernel
KERNEL_NAME = ptr_chase

TILES_X ?= 16
TILES_Y ?= 8
TILE_GROUP_DIM_X := $(TILES_X)
TILE_GROUP_DIM_Y := $(TILES_Y)

###############################################################################
# Host code compilation flags and flow
###############################################################################

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = main.cpp
BLOCK_DIM = 16
DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -D_DEFAULT_SOURCE -DBLOCK_DIM=$(BLOCK_DIM)
DEFINES += -DTILE_GROUP_DIM_X=$(TILE_GROUP_DIM_X)
DEFINES += -DTILE_GROUP_DIM_Y=$(TILE_GROUP_DIM_Y)
CDEFINES += 
CXXDEFINES += 

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=gnu99 $(FLAGS)
CXXFLAGS += -std=c++2a $(FLAGS) -O3

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

RISCV_INCLUDES += -I.
# To switch between g++ and clang, uncomment the line below. g++ is
# the default. To view the disassembly, type `make kernel.dis`

# Clang does not work as well as GCC in this instance. It seems to refuse to inline...
#RISCV_CXX = $(RISCV_CLANGXX)
RISCV_LINK_SCRIPT=../bsg_link.ld
kernel.riscv: kernel.rvo

# Tile Group Dimensions
RISCV_CCPPFLAGS += -O3
RISCV_DEFINES += -Dbsg_tiles_X=$(TILE_GROUP_DIM_X)
RISCV_DEFINES += -Dbsg_tiles_Y=$(TILE_GROUP_DIM_Y)
RISCV_DEFINES += -DBLOCK_DIM=$(BLOCK_DIM)

include $(EXAMPLES_PATH)/cuda/riscv.mk

###############################################################################
# Execution flow
#
# C_ARGS: Use this to pass arguments that you want to appear in argv
#         For SPMD tests C arguments are: <Path to RISC-V Binary> <Test Name>
#
# SIM_ARGS: Use this to pass arguments to the simulator
###############################################################################
C_ARGS ?= $(BSG_MANYCORE_KERNELS) $(KERNEL_NAME)

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


###############################################################################
# Sweep flow
###############################################################################

threads-from-name     = $(firstword $(subst _, ,$(filter %_threads,$(subst __, ,$1))))
tx-from-name          = $(subst tx, ,$(word 1, $(subst _, ,$(filter %_threads,$(subst __, ,$1)))))
ty-from-name          = $(subst ty, ,$(word 2, $(subst _, ,$(filter %_threads,$(subst __, ,$1)))))
work-from-name        = $(firstword $(subst _, ,$(filter %_work,$(subst __, ,$1))))
offset-from-name      = $(firstword $(subst _, ,$(filter %_offset,$(subst __, ,$1))))
logsz-from-name       = $(firstword $(subst _, ,$(filter %_logsz,$(subst __, ,$1))))

test-name = $(1)_threads__$(2)_work__$(3)_offset__$(4)_logsz.deploy

# Divergence DRAM Throughput Sweep (LOGSZ = 24, STRIDE = 1)
DD_SWEEP_THREADS = 1tx_1ty 2tx_1ty 4tx_1ty 8tx_1ty 16tx_1ty 16tx_2ty 16tx_4ty 16tx_8ty
DD_SWEEP_WORK = 1 2 4 8 16

$(foreach t,$(DD_SWEEP_THREADS),\
$(foreach w,$(DD_SWEEP_WORK),\
$(eval DD_TESTS += $(call test-name,$t,$w,65535,27))))

# Divergence Cache Throughput Sweep (LOGSZ = 18, STRIDE = 1)
DC_SWEEP_THREADS = 1tx_1ty 2tx_1ty 4tx_1ty 8tx_1ty 16tx_1ty 16tx_2ty 16tx_4ty 16tx_8ty
DC_SWEEP_WORK = 1 2 4 8 16

$(foreach t,$(DC_SWEEP_THREADS),\
$(foreach w,$(DC_SWEEP_WORK),\
$(eval DC_TESTS += $(call test-name,$t,$w,128,18))))

# Latency DRAM Sweep (LOGSZ = 27, STRIDE = 1)
LD_SWEEP_THREADS = 16tx_8ty
LD_SWEEP_WORK = 1 2 4 8 16

$(foreach t,$(LD_SWEEP_THREADS),\
$(foreach w,$(LD_SWEEP_WORK),\
$(eval LD_TESTS += $(call test-name,$t,$w,65535,27))))

# Latency Cache Throughput Sweep (LOGSZ = 18, STRIDE = 1)
LC_SWEEP_THREADS = 16tx_8ty
LC_SWEEP_WORK = 1 2 4 8 16

$(foreach t,$(LC_SWEEP_THREADS),\
$(foreach w,$(LC_SWEEP_WORK),\
$(eval LD_TESTS += $(call test-name,$t,$w,128,18))))


go:$(DD_TESTS) $(LD_TESTS)

cleanfollow:
	rm -rf $(FOLLOW)

follow: $(FOLLOW)

%.deploy:
	mkdir $@
	echo "TILES_X = $(call tx-from-name,$*)">> $@/Makefile
	echo "TILES_Y = $(call ty-from-name,$*)">> $@/Makefile
	echo "DEFINES += -DARR_LOG2_NUM_ELEMENTS=$(call logsz-from-name,$*)" >> $@/Makefile
	echo "DEFINES += -DOFFSET_STRIDE=$(call offset-from-name,$*)" >> $@/Makefile
	echo "DEFINES += -DLATENCY_WORK=$(call work-from-name,$*)" >> $@/Makefile
	echo "RISCV_DEFINES +=-DARR_LOG2_NUM_ELEMENTS=$(call logsz-from-name,$*)" >> $@/Makefile
	echo "RISCV_DEFINES +=-DLATENCY_WORK=$(call work-from-name,$*)" >> $@/Makefile
	echo "VPATH =.." >> $@/Makefile
	cat Makefile >> $@/Makefile
	$(MAKE) -C $@ kernel.dis > $@/kernel.dis
	$(MAKE) -C $@ stats

cleansweep:
	rm -rf *.deploy
