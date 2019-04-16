include ../../../Makefile.environment
include $(CL_DIR)/Makefile.dimensions

TESTS_PATH=$(REGRESSION_PATH)/library/
include $(TESTS_PATH)/Makefile.tests

# The HDK Makefile must come before Makefile.include (to override their clean)
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
include ../Makefile.include
