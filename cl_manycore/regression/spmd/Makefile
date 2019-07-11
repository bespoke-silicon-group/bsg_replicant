include ../../Makefile.environment
include Makefile.tests
include $(CL_DIR)/Makefile.machine.include
include $(REGRESSION_PATH)/Makefile.common

.PHONY: help
.DEFAULT_GOAL =  help

help:
	@echo "To make a specific test run: make <testname> and then execute the resulting binary"

$(USER_RULES): test_%.rule: $(BSG_MANYCORE_DIR)/software/spmd/%/main.riscv

$(USER_CLEAN_RULES): test_%.clean: %.clean
%.clean:
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst .clean,,$@) clean

$(BSG_MANYCORE_DIR)/software/spmd/%/main.riscv: $(CL_DIR)/Makefile.machine.include
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	bsg_tiles_X=$(TILE_GROUP_DIM_X) \
	bsg_tiles_Y=$(TILE_GROUP_DIM_Y) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -C $(dir $@) $(notdir $@)


