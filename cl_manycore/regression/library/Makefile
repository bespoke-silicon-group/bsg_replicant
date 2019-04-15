CFLAGS  := -std=c11
CXXFLAGS:= -std=c++11 -pthread -O0 $(LIBRARY_REGRESSION_DEFINES)
LDFLAGS := -lbsg_manycore_runtime
include ../../Makefile.dimensions
include Makefile.tests

C_TARGETS   := $(LIBRARY_C_REGRESSION_TESTS)
CXX_TARGETS := $(LIBRARY_CXX_REGRESSION_TESTS)
TARGETS := $(C_TARGETS) $(CXX_TARGETS)

all: $(TARGETS)

$(C_TARGETS): %: %.h
$(CXX_TARGETS): %: %.hpp

# each target 't' in TARGETS relies on an object file 't.o'
$(TARGETS): %: %.o 

# ensure correct linking by using C/C++ linker frontend
$(C_TARGETS): LD:=$(CC)
$(CXX_TARGETS): LD:=$(CXX)

# linking rule for all targets
$(TARGETS):
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS) 

clean: 
	rm -f *.o $(TARGETS) *.regression.log

.PHONY: all clean

include ../Makefile.include
