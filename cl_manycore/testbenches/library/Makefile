include ../../Makefile.environment
include $(CL_DIR)/Makefile.dimensions

.DEFAULT_GOAL=cosim

# REGRESSION_PATH and TESTBENCH_PATH are set in Makefile.environment
TESTS_PATH=$(REGRESSION_PATH)/library/
SIM_PATH=$(TESTBENCH_PATH)/library/

include $(TESTS_PATH)/Makefile.tests

# The HDK Makefile must come before Makefile.common to override AWS' (broken) clean
# HDK_COMMON_DIR is set by running `source hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include $(TESTBENCH_PATH)/Makefile.common
