include Makefile.tests
include ../../Makefile.dimensions
include ../Makefile.common

.PHONY: autorebuild

autorebuild:

$(USER_RULES): test_%.rule: $(BSG_MANYCORE_DIR)/software/spmd/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/%/binary: autorebuild
	$(MAKE) -C $(dir $@) clean main.riscv \
		bsg_tiles_X=$(CL_MANYCORE_DIM_X) \
		bsg_tiles_Y=$(CL_MANYCORE_DIM_Y) \
		IGNORE_CADENV=1 \
		LINK_SCRIPT=$(BSG_MANYCORE_DIR)/software/spmd/common/link_dmem2.ld


