CFLAGS  := -std=c11
CXXFLAGS:= -std=c++11 -pthread -O0 $(SPMD_REGRESSION_DEFINES)
LDFLAGS := -lbsg_manycore_runtime
include ../../Makefile.dimensions
include Makefile.tests

all: $(TARGETS)

C_TARGETS   := $(SPMD_C_REGRESSION_TESTS)
CXX_TARGETS := $(SPMD_CXX_REGRESSION_TESTS)
TARGETS := $(C_TARGETS) $(CXX_TARGETS)

$(C_TARGETS): %: %.h
$(CXX_TARGETS): %: %.hpp

# each target 't' in TARGETS relies on an object file 't.o'
$(TARGETS): %: %.o
$(TARGETS): test_%: $(BSG_MANYCORE_DIR)/software/spmd/%/binary

# ensure correct linking by using C/C++ linker frontend
$(C_TARGETS): LD:=$(CC)
$(CXX_TARGETS): LD:=$(CXX)

# linking rule for all targets
$(TARGETS):
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS) 

$(BSG_MANYCORE_DIR)/software/spmd/%/binary:
	$(MAKE) -C $(dir $@) clean main.riscv \
		bsg_tiles_X=$(CL_MANYCORE_DIM_X) \
		bsg_tiles_Y=$(CL_MANYCORE_DIM_Y)

clean:
	rm -f *.o $(TARGETS) *.regression.log

.PHONY: all clean 

include ../Makefile.include
