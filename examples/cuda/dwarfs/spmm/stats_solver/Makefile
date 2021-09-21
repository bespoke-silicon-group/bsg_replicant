REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

solver: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/Random.hpp
solver: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/EigenSparseMatrix.hpp
solver: $(APPLICATION_PATH)/include/host/Solver.hpp
solver: $(EXAMPLES_PATH)/cuda/dwarfs/src/Random.cpp
solver: $(APPLICATION_PATH)/src/host/solver.cpp
	$(eval INCLUDES+=-I$(EXAMPLES_PATH)/cuda/dwarfs/include/host)
	$(eval INCLUDES+=-I$(APPLICATION_PATH)/include/host)
	$(eval INCLUDES+=-I$(EXAMPLES_PATH)/cuda/dwarfs/imports/eigen)
	$(CXX) -std=c++11 $(INCLUDES) -o $@ $(filter %.cpp,$^)


include $(APPLICATION_PATH)/inputs.mk

$(foreach input,$(INPUTS),$(input).stats): %.stats: inputs solver
	./solver $($*) $($*__directed) $($*__weighted) $($*__zero-indexed) $@.solve_row.csv \
	$*.input.csv $*.solution.csv $*.answer.csv

clean:
	rm -rf solver *.input.csv *.solve_row.csv *.solution.csv *.answer.csv
