REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk
SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd

ifndef SPMD_NAME
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: SPMD_NAME is not defined$(NC)"))
endif

###############################################################################
# Host code compilation flags and flow
###############################################################################
POD_GROUP_X ?= 1
POD_GROUP_Y ?= 1
TILE_GROUP_DIM_X ?= 1
TILE_GROUP_DIM_Y ?= 1
DEVICE_ID ?= -1

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = $(EXAMPLES_PATH)/spmd-vlsi24/loader.c

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -D_DEFAULT_SOURCE
CDEFINES += -DPOD_GROUP_X=$(POD_GROUP_X)
CDEFINES += -DPOD_GROUP_Y=$(POD_GROUP_Y)
CXXDEFINES += 

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
C_ARGS ?= $(BSG_MANYCORE_KERNELS) $(SPMD_NAME) $(TILE_GROUP_DIM_X) $(TILE_GROUP_DIM_Y) $(DEVICE_ID)

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
	rm -f $(EXAMPLES_PATH)/spmd-vlsi24/loader.o


