#INCLUDES = -I../drivers/ -I../libraries/
CFLAGS  := -std=c11
CXXFLAGS:= -std=c++11 -pthread -O0
LDFLAGS := -lbsg_manycore_runtime


# if you have written a program such as <my-program>.c or <my-program>.cpp
# add <my-program> in the appropriate list of targets below
CUDA_TARGETS := cuda_add_kernel_one_tile_0_1 cuda_add_kernel_one_tile_2_2 cuda_add_kernel_one_tile_0_1_twice cuda_add_kernel_four_tiles_0_1 cuda_add_kernel_four_tiles_2_2 cuda_add_kernel_four_tiles_2_2_twice cuda_stack_arguments 
C_TARGETS   := loopback_test read_write_test load_vector_test $(CUDA_TARGETS)
CXX_TARGETS := 

TARGETS := $(C_TARGETS) $(CXX_TARGETS)
HEADERS := hb_mc_print_hex.h

all: $(TARGETS)

# all objects depend on all headers
$(wilcard *.o): $(HEADERS)

# each target 't' in TARGETS relies on an object file 't.o'
$(TARGETS): %: %.o

# ensure correct linking by using C/C++ linker frontend
$(C_TARGETS):   LD:=$(CC)
$(CXX_TARGETS): LD:=$(CXX)

# linking rule for all targets
$(TARGETS):
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS)

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean 
