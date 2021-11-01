REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm
include $(APPLICATION_PATH)/utils.mk

solver: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/Random.hpp
solver: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/EigenSparseMatrix.hpp
solver: $(APPLICATION_PATH)/include/host/Solver.hpp
solver: $(EXAMPLES_PATH)/cuda/dwarfs/src/Random.cpp
solver: $(APPLICATION_PATH)/src/host/solver.cpp
	$(eval INCLUDES+=-I$(EXAMPLES_PATH)/cuda/dwarfs/include/host)
	$(eval INCLUDES+=-I$(APPLICATION_PATH)/include/host)
	$(eval INCLUDES+=-I$(EXAMPLES_PATH)/cuda/dwarfs/imports/eigen)
	$(CXX) -std=c++11 $(INCLUDES) -o $@ $(filter %.cpp,$^)

partition_solver: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/Random.hpp
partition_solver: $(EXAMPLES_PATH)/cuda/dwarfs/include/host/EigenSparseMatrix.hpp
partition_solver: $(APPLICATION_PATH)/include/host/Solver.hpp
partition_solver: $(EXAMPLES_PATH)/cuda/dwarfs/src/Random.cpp
partition_solver: $(APPLICATION_PATH)/src/host/partition_solver.cpp
	$(eval INCLUDES+=-I$(EXAMPLES_PATH)/cuda/dwarfs/include/host)
	$(eval INCLUDES+=-I$(APPLICATION_PATH)/include/host)
	$(eval INCLUDES+=-I$(EXAMPLES_PATH)/cuda/dwarfs/imports/eigen)
	$(CXX) -std=c++11 $(INCLUDES) -o $@ $(filter %.cpp,$^)



include $(APPLICATION_PATH)/inputs.mk

$(foreach input,$(INPUTS),$(input).stats): %.stats: inputs solver
	./solver $($*) $($*__directed) $($*__weighted) $($*__zero-indexed) $@.solve_row.csv \
	$*.input.csv $*.solution.csv $*.answer.csv \
	$*.A_stats.csv $*.B_stats.csv

PARTITION_FACTOR = 8

$(foreach input,$(INPUTS),$(input).partition.stats): %.partition.stats: inputs partition_solver
	./partition_solver $($*) $($*__directed) $($*__weighted) $($*__zero-indexed) $@.solve_row.csv \
	$*.input.csv $*.solution.csv $*.answer.csv \
	$*.A_stats.csv $*.B_stats.csv \
	$(PARTITION_FACTOR)
	$(eval parts\
	= $(call cartesian\
	,$(call range,0,$(PARTITION_FACTOR))\
	,$(call range,0,$(PARTITION_FACTOR))\
	,_))
	$(eval part_stats = $(foreach part,$(parts),$(part).$@.solve_row.csv))
	python3 $(APPLICATION_PATH)/py/summarize_partitions.py $*.solve_row.partitions.summary.csv $(part_stats)

$(foreach input,$(INPUTS),$(input).memcount): %.memcount:
	@echo $*:
	@printf "%e rows\n" $($*__rows)
	@printf "%e columns\n" $($*__cols)
	@printf "%e nonzeros\n" $($*__nnz)
	@printf "%e solution nonzeros\n" $($*__solnnz)
	@$(eval matrices=3)
	@$(eval omatrices=2)
	@$(eval imatrices=2)
	@$(eval wordbytes=4)
	$(eval rbytes=$(shell echo $($*__rows)*2*$(wordbytes)*$(matrices)+$($*__rows)*$(wordbytes)|bc))
	$(eval nzbytes=$(shell echo $($*__nnz)*2*$(wordbytes)*$(omatrices)|bc))
	$(eval solnzbytes=$(shell echo $($*__solnnz)*4*$(wordbytes)*$(imatrices)|bc))
	$(eval total=$(shell echo $(rbytes)+$(nzbytes)+$(solnzbytes)|bc))
	@printf "%e bytes\n" $(total)

clean:
	rm -rf solver *.csv partition_solver
