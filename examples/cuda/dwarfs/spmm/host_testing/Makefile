REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

TARGETS := partition

CXXFLAGS += -std=c++11 -O3
CXXFLAGS += -I$(EXAMPLES_PATH)/cuda/dwarfs/include/host
CXXFLAGS += -I$(APPLICATION_PATH)/include/host
CXXFLAGS += -I$(EXAMPLES_PATH)/cuda/dwarfs/imports/eigen

partition: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/EigenSparseMatrix.hpp
partition: partition.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(LDFLAGS)

include $(APPLICATION_PATH)/inputs.mk

PARTITION_TESTS = $(foreach input,$(INPUTS),partition.$(input))
$(PARTITION_TESTS): partition.%: inputs partition
	./partition $($*) $($*__directed) $($*__weighted) $($*__zero-indexed) $@ | tee $@

clean:
	rm -f $(PARTITION_TESTS)
	rm -f $(TARGETS)
	rm -f *.el
	rm -f *.csv
