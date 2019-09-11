include ../../environment.mk
include $(CL_DIR)/hdk.mk

include $(CL_DIR)/Makefile.machine.include
include $(REGRESSION_PATH)/cuda/tests.mk
# The HDK Makefile must come before cosimulation.mk to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include $(TESTBENCH_PATH)/cosimulation.mk
# Makefile.rules must come after cosimulation.mk so that <test_name>.rule and
# <test_name>.clean are not overwritten.
include $(REGRESSION_PATH)/cuda/rules.mk

.DEFAULT_GOAL=regression
