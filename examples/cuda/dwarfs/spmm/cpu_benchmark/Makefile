REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

MKLROOT=/opt/intel/oneapi/mkl/2021.3.0

LDFLAGS += -Wl,--start-group
LDFLAGS += $(MKLROOT)/lib/intel64/libmkl_intel_ilp64.a
LDFLAGS += $(MKLROOT)/lib/intel64/libmkl_gnu_thread.a
LDFLAGS += $(MKLROOT)/lib/intel64/libmkl_core.a
LDFLAGS += -Wl,--end-group -lgomp -lpthread -lm -ldl

CXXFLAGS += -std=c++11 -O3
CXXFLAGS += -DMKL_ILP64  -m64  -I"$(MKLROOT)/include"
CXXFLAGS += -I$(EXAMPLES_PATH)/cuda/dwarfs/include/host
CXXFLAGS += -I$(APPLICATION_PATH)/include/host
CXXFLAGS += -I$(EXAMPLES_PATH)/cuda/dwarfs/imports/eigen

cpu_benchmark: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/Random.hpp
cpu_benchmark: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/EigenSparseMatrix.hpp
cpu_benchmark: $(EXAMPLES_PATH)/cuda/dwarfs/src/Random.cpp
cpu_benchmark: $(APPLICATION_PATH)/src/host/cpu_benchmark.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(LDFLAGS)


include $(APPLICATION_PATH)/inputs.mk

ITERATIONS=10
THREADS=16

CPU_BENCHMARK = $(foreach input, $(INPUTS),$(input).run)
$(CPU_BENCHMARK): %.run: inputs cpu_benchmark
	OMP_NUM_THREADS=$(THREADS) ./cpu_benchmark $($*) $($*__directed) $($*__weighted) $($*__zero-indexed) $(ITERATIONS) | tee $@

run: $(CPU_BENCHMARK)

clean:
	rm -rf cpu_benchmark
