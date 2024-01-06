REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
SPMD_NAME := asic-02-noc-intrapod
POD_GROUP_X=4
POD_GROUP_Y=4
include $(REPLICANT_PATH)/environment.mk
SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd
include $(EXAMPLES_PATH)/spmd/loader.mk

