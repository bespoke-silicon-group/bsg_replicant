include ../../Makefile.environment
SRC_PATH=$(REGRESSION_PATH)/library/
SIM_PATH=$(TESTBENCH_PATH)/library/

include $(CL_DIR)/Makefile.machine.include
include $(REGRESSION_PATH)/library/Makefile.tests
# The HDK Makefile must come before Makefile.common to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc

include $(TESTBENCH_PATH)/Makefile.common

.DEFAULT_GOAL=regression
