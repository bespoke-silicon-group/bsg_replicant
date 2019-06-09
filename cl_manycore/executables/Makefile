CXXFLAGS := -std=c++11 -O3
LDFLAGS  := -lbsg_manycore_runtime -lm
EXCECUTABLES := bsg_manycore_spmd_loader
DEBUG_OBJECTS:= bsg_manycore_run_spmd_program.o

.PHONY: all clean

all: $(EXCECUTABLES)

$(DEBUG_OBJECTS): CXXFLAGS += -DDEBUG

$(wildcard *.o): $(wildcard *.h) $(wildcard *.hpp)
%.o: %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $<

bsg_manycore_spmd_loader: bsg_manycore_run_spmd_program.o
bsg_manycore_spmd_loader: bsg_manycore_spmd_loader.o
bsg_manycore_spmd_loader:
	$(CXX) $(LDFLAGS) -o $@ $(filter %.o, $^)

clean:
	rm -rf $(EXECUTABLES) *.o *~
