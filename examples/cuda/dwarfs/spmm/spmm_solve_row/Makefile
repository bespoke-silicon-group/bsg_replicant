REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_input__$(2)_row__$(3)_algorithm__$(4)_dmem-words
get-input       = $(firstword $(subst _, ,$(filter %_input,$(subst __, ,$1))))
get-row		= $(firstword $(subst _, ,$(filter %_row,$(subst __, ,$1))))
get-algorithm	= $(firstword $(subst _, ,$(filter %_algorithm,$(subst __, ,$1))))
get-dmem-words  = $(firstword $(subst _, ,$(filter %_dmem-words,$(subst __, ,$1))))

DMEM = 0

08blocks-rows = 0 297 35 184
$(foreach row,$(08blocks-rows),\
$(eval TESTS += $(call test-name,08blocks,$(row),hash-table,$(DMEM)))\
$(eval TESTS += $(call test-name,08blocks,$(row),insertion-sort,$(DMEM))))

# select rows with known number of nonzeros in the output
# sorted by nnz, nnz increments of 50
# 0-450
wiki-vote-rows := 0 387 42 830 497 478 3315 192 2158 5117

# insertion-sort breaks after 450
$(foreach row,$(wiki-vote-rows),\
$(eval TESTS += $(call test-name,wiki-vote,$(row),insertion-sort)))

# hash-table can handle any number of nonzeros
# sorted by nnz, imcrements of 100
# 500-900
wiki-vote-rows += 1570 262 3621 3226 2434
# sorted by nnz, imcrements of 250
# 1000-2000
wiki-vote-rows += 2273 1584 2706 1469 1373
$(foreach row,$(wiki-vote-rows),\
$(eval TESTS += $(call test-name,wiki-vote,$(row),hash-table,$(DMEM)))\
$(eval TESTS += $(call test-name,wiki-vote,$(row),fmult-only,$(DMEM)))\
$(eval TESTS += $(call test-name,wiki-vote,$(row),hash-table-no-sort,$(DMEM))))

define get-test-parameters
$(eval INPUT=$(call get-input,$1))
$(eval ROW=$(call get-row,$1))
$(eval ALGORITHM=$(call get-algorithm,$1))
$(eval DMEM=$(call get-dmem-words,$1))
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
@echo INPUT=$(INPUT)		>> $2
@echo ROW=$(ROW)		>> $2
@echo ALGORITHM=$(ALGORITHM)	>> $2
@echo DMEM=$(DMEM)              >> $2
endef

# This can be overriden to set a custom simulation directory for a test.
# Defaults to $(APPLICATION_PATH)/$(test-name)
#
# See spmm for examples of overriding
define get-sim-dir-from-test
$(eval SIMULATION_DIR=$(APPLICATION_PATH)/spmm_solve_row/$1)
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk

plots: $(addsuffix .plots,$(TESTS))
$(addsuffix .plots,$(TESTS)): %.plots: %
	$(MAKE) -C $* plots

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/vcache_solve_row.py $^

dramsim3.summary.csv: $(addsuffix /dramsim3.tag.json,$(TESTS))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/dramsim3_solve_row.py $^



