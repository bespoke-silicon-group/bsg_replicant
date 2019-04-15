include ../../../Makefile.environment
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc

TESTS_PATH=$(CL_DIR)/regression/spmd/
include $(CL_DIR)/Makefile.dimensions
include $(TESTS_PATH)/Makefile.tests
include ../Makefile.include

$(SIM_TARGETS): test_%.cosim.log: $(BSG_MANYCORE_DIR)/software/spmd/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/%/binary:
	$(MAKE) -C $(TESTS_PATH) $@ 
