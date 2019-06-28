include ../../Makefile.environment
TESTS_PATH=$(REGRESSION_PATH)/cuda/
SIM_PATH=$(TESTBENCH_PATH)/cuda/

include $(CL_DIR)/Makefile.machine.include
include $(REGRESSION_PATH)/cuda/Makefile.tests
# The HDK Makefile must come before Makefile.common to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include $(TESTBENCH_PATH)/Makefile.common

.DEFAULT_GOAL=cosim

$(USER_RULES): test_%.rule: $(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/main.riscv

$(USER_CLEAN_RULES): test_%.clean: %.clean
%.clean:
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -C $(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/$(subst .clean,,$@) clean

$(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/main.riscv: %.clean
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	bsg_tiles_X=$(TILE_GROUP_DIM_X) \
	bsg_tiles_Y=$(TILE_GROUP_DIM_Y) \
	$(MAKE) -C $(dir $@) $(notdir $@)
