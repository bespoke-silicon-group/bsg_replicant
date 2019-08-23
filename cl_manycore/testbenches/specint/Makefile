include ../../Makefile.environment
include $(CL_DIR)/Makefile.hdk
SRC_PATH=$(REGRESSION_PATH)/specint
SIM_PATH=$(TESTBENCH_PATH)/specint

include $(CL_DIR)/Makefile.machine.include
include $(REGRESSION_PATH)/specint/Makefile.tests
# The HDK Makefile must come before Makefile.common to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include $(TESTBENCH_PATH)/Makefile.common
# Makefile.rules must come after Makefile.common so that <test_name>.rule and
# <test_name>.clean are not overwritten.
include $(REGRESSION_PATH)/specint/Makefile.rules

.DEFAULT_GOAL=regression
