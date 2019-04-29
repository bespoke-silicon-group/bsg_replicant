include Makefile.tests
include ../../Makefile.dimensions
include ../Makefile.common

$(TARGETS): test_%: $(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/binary

$(BSG_MANYCORE_DIR)/software/spmd/bsg_cuda_lite_runtime/%/binary:
	$(MAKE) -C $(dir $@) clean main.riscv 
		bsg_tiles_X=2 \
		bsg_tiles_Y=2 \
		bsg_global_X=4 \
		bsg_global_Y=4 \
		bsg_tiles_org_X=0 \
		bsg_tiles_org_Y=1


