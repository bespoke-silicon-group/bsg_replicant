export POD_GROUP_X := 4
export POD_GROUP_Y := 2
export TILE_GROUP_DIM_X := 16
export TILE_GROUP_DIM_Y := 8
export ITERATIONS := 10

REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
SPMD_NAME := $(notdir $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))))

include $(REPLICANT_PATH)/environment.mk

SPMD_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd

include $(EXAMPLES_PATH)/spmd-vlsi24-2k/loader.mk

sweep:
	mkdir results
	sudo python3 -u sweep.py | tee -i results/run.log
	sudo chown -R $(USER):$(USER) results/

confirm:
	mkdir confirm
	sudo python3 -u confirm.py | tee -i confirm/run.log
	sudo chown -R $(USER):$(USER) confirm/
