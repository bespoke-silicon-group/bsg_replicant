include Makefile.tests
include ../../Makefile.dimensions
include ../Makefile.common

.PHONY: autorebuild

autorebuild:

$(USER_RULES): test_%.rule: $(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/binary: autorebuild
	$(MAKE) -C $(dir $@) clean main.riscv \
		bsg_tiles_X=$(TILE_GROUP_DIM_X) \
		bsg_tiles_Y=$(TILE_GROUP_DIM_Y) \
		IGNORE_CADENV=1

