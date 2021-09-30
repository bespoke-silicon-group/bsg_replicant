REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name               = $(1)_input__$(2)_tx__$(3)_ty__$(4)_solve-row-alg__$(5)_comp-offs-alg__$(6)_slv-row-dmem-words
get-input		= $(firstword $(subst _, ,$(filter %_input,$(subst __, ,$1))))
get-tx			= $(firstword $(subst _, ,$(filter %_tx,$(subst __, ,$1))))
get-ty			= $(firstword $(subst _, ,$(filter %_ty,$(subst __, ,$1))))
get-solve-row-alg	= $(firstword $(subst _, ,$(filter %_solve-row-alg,$(subst __, ,$1))))
get-comp-offs-alg	= $(firstword $(subst _, ,$(filter %_comp-offs-alg,$(subst __, ,$1))))
get-slv-row-dmem-words	= $(firstword $(subst _, ,$(filter %_slv-row-dmem-words,$(subst __, ,$1))))

RUN-INPUTS = 08blocks wiki-vote
DMEM_WORDS = 0 $(shell echo 7*128|bc)
COMP_OFFS_ALG  = sum-tree
SOLVE_ROW_ALG  = hash-table

TX-TY = 1-1 16-8
tx = $(firstword $(subst -, ,$1))
ty = $(lastword  $(subst -, ,$1))

$(foreach input,$(RUN-INPUTS),\
$(foreach solve-row,$(SOLVE_ROW_ALG),\
$(foreach comp-offs,$(COMP_OFFS_ALG),\
$(foreach tx-ty,$(TX-TY),\
$(foreach dmem,$(DMEM_WORDS),\
$(eval TESTS += $(call test-name,$(input),$(call tx,$(tx-ty)),$(call ty,$(tx-ty)),$(solve-row),$(comp-offs),$(dmem))))))))

define get-test-parameters
$(eval INPUT=$(call get-input,$1))
$(eval TX=$(call get-tx,$1))
$(eval TY=$(call get-ty,$1))
$(eval SOLVE_ROW_ALG=$(call get-solve-row-alg,$1))
$(eval COMP_OFFS_ALG=$(call get-comp-offs-alg,$1))
$(eval SOLVE_ROW_DMEM_WORDS=$(call get-slv-row-dmem-words,$1))
endef

# dwarfs should define this function hook to add
# app specific parameters
# 1: test-name
# 2: parameters.mk target
#
# $(2) is set to the parameters.mk of the test directory
# typically this is $(APPLICATION_PATH)/$(test-name)/parameters.mk
define parameters-mk-add-application-params
$(eval $(call get-test-parameters,$1))
@echo INPUT=$(INPUT)					>> $2
@echo TX=$(TX)						>> $2
@echo TY=$(TY)						>> $2
@echo SOLVE_ROW_ALG=$(SOLVE_ROW_ALG)			>> $2
@echo COMP_OFFS_ALG=$(COMP_OFFS_ALG)			>> $2
@echo SOLVE_ROW_DMEM_WORDS=$(SOLVE_ROW_DMEM_WORDS)	>> $2
endef


# This can be overriden to set a custom simulation directory for a test.
# Defaults to $(APPLICATION_PATH)/$(test-name)
#
# See spmm for examples of overriding
define get-sim-dir-from-test
$(eval SIMULATION_DIR=$(APPLICATION_PATH)/spmm/$1)
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk

plots: $(addsuffix .plots,$(TESTS))
$(addsuffix .plots,$(TESTS)): %.plots: %
	$(MAKE) -C $* plots

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/vcache.py $^

dramsim3.summary.csv: $(addsuffix /dramsim3.tag.json,$(TESTS))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/dramsim3.py $^

purge: clean.inputs

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
	./solver $($*) $($*__directed) $($*__weighted) $($*__zero-indexed) $@.solve_row.csv

clean.inputs.stats:
	rm -rf solver *.solve_row.csv

purge: clean.inputs.stats
