REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_table-words__$(2)_block-words__$(3)_groups__$(4)_tgx__$(5)_tgy
get-table-words = $(firstword $(subst _, ,$(filter %_table-words,$(subst __, ,$1))))
get-block-words = $(firstword $(subst _, ,$(filter %_block-words,$(subst __, ,$1))))
get-groups      = $(firstword $(subst _, ,$(filter %_groups,$(subst __, ,$1))))
get-tgx         = $(firstword $(subst _, ,$(filter %_tgx,$(subst __, ,$1))))
get-tgy         = $(firstword $(subst _, ,$(filter %_tgy,$(subst __, ,$1))))

64K=$(shell echo 64*1024|bc)

#TESTS += $(call test-name,1024,8,1,1,1)
#TESTS += $(call test-name,$(64K),8,32,1,4)
#TESTS += $(call test-name,1024,8,32,1,4)
#TESTS += $(call test-name,$(64K),8,1,16,8)
TESTS += $(call test-name,$(64K),8,1,16,8)

define get-test-parameters
$(eval TABLE_WORDS=$(call get-table-words,$1))
$(eval BLOCK_WORDS=$(call get-block-words,$1))
$(eval GROUPS=$(call get-groups,$1))
$(eval TGX=$(call get-tgx,$1))
$(eval TGY=$(call get-tgy,$1))
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
@echo BLOCK_WORDS=$(BLOCK_WORDS) >> $2
@echo GROUPS=$(GROUPS) >> $2
@echo TGX=$(TGX) >> $2
@echo TGY=$(TGY) >> $2
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/stream

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk
