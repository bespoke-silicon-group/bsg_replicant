include ../../../Makefile.environment
include $(CL_DIR)/Makefile.dimensions

.DEFAULT_GOAL=cosim

# REGRESSION_PATH is set in Makefile.environment
TESTS_PATH=$(REGRESSION_PATH)/cuda/
include $(TESTS_PATH)/Makefile.tests

# The HDK Makefile must come before Makefile.common to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include ../Makefile.common

$(SIM_LOG_TARGETS): test_%.cosim.log: $(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/binary:
	$(MAKE) -C $(TESTS_PATH) $@ 
