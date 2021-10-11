REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_input__$(2)_row__$(3)_dmem-words__$(4)_hash-table-words
get-input       = $(firstword $(subst _, ,$(filter %_input,$(subst __, ,$1))))
get-row		= $(firstword $(subst _, ,$(filter %_row,$(subst __, ,$1))))
get-dmem-words  = $(firstword $(subst _, ,$(filter %_dmem-words,$(subst __, ,$1))))
get-hash-table-words = $(firstword $(subst _, ,$(filter %_hash-table-words,$(subst __, ,$1))))

DMEMS = 0 $(shell echo 128*6|bc)
#NONZEROS_TABLE_WORDSS = $(shell echo 32*1024|bc) 1024
NONZEROS_TABLE_WORDSS = 1024

wiki-vote-rows +=   0   #    0 updates
wiki-vote-rows +=  31   #   32 updates
wiki-vote-rows += 174   #   64 updates
wiki-vote-rows += 1547  #   96 updates
wiki-vote-rows += 240   #  128 updates
wiki-vote-rows += 265   #  160 updates
wiki-vote-rows += 295   #  192 updates
wiki-vote-rows += 1057  #  224 updates
wiki-vote-rows += 97    #  256 updates
wiki-vote-rows += 3016  #  384 updates
wiki-vote-rows += 5514  #  512 updates
wiki-vote-rows += 3742  #  641 updates
wiki-vote-rows += 4446  #  770 updates
wiki-vote-rows += 5177  #  898 updates
wiki-vote-rows += 4122  # 1026 updates

$(foreach dmem,$(DMEMS),\
$(foreach tblw,$(NONZEROS_TABLE_WORDSS),\
$(foreach row,$(wiki-vote-rows),\
$(eval TESTS+=$(call test-name,wiki-vote,$(row),$(dmem),$(tblw))))))

define get-test-parameters
$(eval INPUT=$(call get-input,$1))
$(eval ROW=$(call get-row,$1))
$(eval DMEM=$(call get-dmem-words,$1))
$(eval NONZEROS_TABLE_WORDS=$(call get-hash-table-words,$1))
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
@echo DMEM=$(DMEM)              >> $2
@echo NONZEROS_TABLE_WORDS=$(NONZEROS_TABLE_WORDS) >> $2
endef

# This can be overriden to set a custom simulation directory for a test.
# Defaults to $(APPLICATION_PATH)/$(test-name)
#
# See spmm for examples of overriding
define get-sim-dir-from-test
$(eval SIMULATION_DIR=$(APPLICATION_PATH)/hash_table/$1)
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk

plots: $(addsuffix .plots,$(TESTS))
$(addsuffix .plots,$(TESTS)): %.plots: %
	$(MAKE) -C $* plots

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/vcache_hash_table.py $^

dramsim3.summary.csv: $(addsuffix /dramsim3.tag.json,$(TESTS))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/dramsim3_hash_table.py $^



