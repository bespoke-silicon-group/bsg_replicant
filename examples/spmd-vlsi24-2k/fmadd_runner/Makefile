POD_GROUP_X := 1
POD_GROUP_Y := 1
TILE_GROUP_DIM_X := 1
TILE_GROUP_DIM_Y := 1
export ITERATIONS := 100

REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
SPMD_NAME := $(notdir $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

include $(REPLICANT_PATH)/environment.mk

SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd

include $(EXAMPLES_PATH)/spmd-vlsi24-2k/loader.mk
