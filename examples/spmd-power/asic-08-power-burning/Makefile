POD_GROUP_X ?= 4
POD_GROUP_Y ?= 4
TILE_GROUP_DIM_X ?= 16
TILE_GROUP_DIM_Y ?= 8


REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
SPMD_NAME := asic-08-power-burning
include $(REPLICANT_PATH)/environment.mk
SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd
include $(EXAMPLES_PATH)/spmd-power/loader.mk

wave:
	dve -full64 -vpd debug.vpd &
