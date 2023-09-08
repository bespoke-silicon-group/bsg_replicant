REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
SPMD_NAME := asic-06-icache_miss
POD_GROUP_X=4
POD_GROUP_Y=2
include $(REPLICANT_PATH)/environment.mk
SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd
include $(EXAMPLES_PATH)/spmd-multipod/loader.mk

