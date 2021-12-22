REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_table-words__$(2)_threads-per-group__$(3)_block-words-per-thread
get-table-words = $(firstword $(subst _, ,$(filter %_table-words,$(subst __, ,$1))))
get-block-words-per-thread = $(firstword $(subst _, ,$(filter %_block-words-per-thread,$(subst __, ,$1))))
get-threads-per-group = $(firstword $(subst _, ,$(filter %_threads-per-group,$(subst __, ,$1))))

64K=$(shell echo 64*1024|bc)
256K=$(shell echo 256*1024|bc)
1M=$(shell echo 1*1024*1024|bc)
4M=$(shell echo 4*1024*1024|bc)
16M=$(shell echo 16*1024*1024|bc)

TESTS += $(call test-name,$(256K),1,1)
TESTS += $(call test-name,$(256K),2,1)
TESTS += $(call test-name,$(256K),4,1)
TESTS += $(call test-name,$(256K),1,2)
TESTS += $(call test-name,$(256K),2,2)
TESTS += $(call test-name,$(256K),4,2)
TESTS += $(call test-name,$(256K),1,4)
TESTS += $(call test-name,$(256K),2,4)
TESTS += $(call test-name,$(256K),4,4)
TESTS += $(call test-name,$(256K),1,8)
TESTS += $(call test-name,$(256K),2,8)
TESTS += $(call test-name,$(256K),4,8)
TESTS += $(call test-name,$(256K),1,16)
TESTS += $(call test-name,$(256K),2,16)
TESTS += $(call test-name,$(256K),4,16)

TESTS += $(call test-name,$(1M),1,1)
TESTS += $(call test-name,$(1M),2,1)
TESTS += $(call test-name,$(1M),4,1)
TESTS += $(call test-name,$(1M),1,2)
TESTS += $(call test-name,$(1M),2,2)
TESTS += $(call test-name,$(1M),4,2)
TESTS += $(call test-name,$(1M),1,4)
TESTS += $(call test-name,$(1M),2,4)
TESTS += $(call test-name,$(1M),4,4)
TESTS += $(call test-name,$(1M),1,8)
TESTS += $(call test-name,$(1M),2,8)
TESTS += $(call test-name,$(1M),4,8)
TESTS += $(call test-name,$(1M),1,16)
TESTS += $(call test-name,$(1M),2,16)
TESTS += $(call test-name,$(1M),4,16)

TESTS += $(call test-name,$(4M),1,1)
TESTS += $(call test-name,$(4M),2,1)
TESTS += $(call test-name,$(4M),4,1)
TESTS += $(call test-name,$(4M),1,2)
TESTS += $(call test-name,$(4M),2,2)
TESTS += $(call test-name,$(4M),4,2)
TESTS += $(call test-name,$(4M),1,4)
TESTS += $(call test-name,$(4M),2,4)
TESTS += $(call test-name,$(4M),4,4)
TESTS += $(call test-name,$(4M),1,8)
TESTS += $(call test-name,$(4M),2,8)
TESTS += $(call test-name,$(4M),4,8)
TESTS += $(call test-name,$(4M),1,16)
TESTS += $(call test-name,$(4M),2,16)
TESTS += $(call test-name,$(4M),4,16)

TESTS += $(call test-name,$(16M),1,1)
TESTS += $(call test-name,$(16M),2,1)
TESTS += $(call test-name,$(16M),4,1)
TESTS += $(call test-name,$(16M),1,2)
TESTS += $(call test-name,$(16M),2,2)
TESTS += $(call test-name,$(16M),4,2)
TESTS += $(call test-name,$(16M),1,4)
TESTS += $(call test-name,$(16M),2,4)
TESTS += $(call test-name,$(16M),4,4)
TESTS += $(call test-name,$(16M),1,8)
TESTS += $(call test-name,$(16M),2,8)
TESTS += $(call test-name,$(16M),4,8)
TESTS += $(call test-name,$(16M),1,16)
TESTS += $(call test-name,$(16M),2,16)
TESTS += $(call test-name,$(16M),4,16)

define get-test-parameters
$(eval TABLE_WORDS=$(call get-table-words,$1))
$(eval BLOCK_WORDS_PER_THREAD=$(call get-block-words-per-thread,$1))
$(eval THREADS_PER_GROUP=$(call get-threads-per-group,$1))
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
@echo TABLE_WORDS=$(TABLE_WORDS) >> $2
@echo BLOCK_WORDS_PER_THREAD=$(BLOCK_WORDS_PER_THREAD) >> $2
@echo THREADS_PER_GROUP=$(THREADS_PER_GROUP) >> $2
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/stream

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk

dramsim3.summary.csv:  $(addsuffix /dramsim3.tag.json,$(TESTS))
	@echo Making $@
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/dramsim3.py $^

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
	@echo Creating $@
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/vcache.py $^

