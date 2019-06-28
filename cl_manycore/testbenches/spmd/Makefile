include ../../Makefile.environment
include $(CL_DIR)/Makefile.dimensions

.DEFAULT_GOAL=cosim

# REGRESSION_PATH is set in Makefile.environment
TESTS_PATH=$(REGRESSION_PATH)/spmd/
SIM_PATH=$(TESTBENCH_PATH)/spmd/

include $(TESTS_PATH)/Makefile.tests

# The HDK Makefile must come before Makefile.common to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include ../Makefile.common

$(USER_RULES): test_%.rule: $(BSG_MANYCORE_DIR)/software/spmd/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/%/binary:
	$(MAKE) -C $(TESTS_PATH) $@ IGNORE_CADENV=1 LINK_SCRIPT=$(BSG_MANYCORE_DIR)/software/spmd/common/link_dmem2.ld
