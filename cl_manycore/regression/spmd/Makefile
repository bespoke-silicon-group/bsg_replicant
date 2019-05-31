include Makefile.tests
include ../../Makefile.dimensions
include ../Makefile.common

$(TARGETS): test_%: $(BSG_MANYCORE_DIR)/software/spmd/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/%/binary:
	$(MAKE) -C $(dir $@) clean main.riscv \
		bsg_tiles_X=$(CL_MANYCORE_DIM_X) \
		bsg_tiles_Y=$(CL_MANYCORE_DIM_Y) \
		IGNORE_CADENV=1


