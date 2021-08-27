REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_rows__$(2)_cols__$(3)_nnz-per-row__$(4)_groups__$(5)_tgx__$(6)_tgy
get-rows        = $(firstword $(subst _, ,$(filter %_rows,$(subst __, ,$1))))
get-cols        = $(firstword $(subst _, ,$(filter %_cols,$(subst __, ,$1))))
get-nnz-per-row = $(firstword $(subst _, ,$(filter %_nnz-per-row,$(subst __, ,$1))))
get-groups      = $(firstword $(subst _, ,$(filter %_groups,$(subst __, ,$1))))
get-tgx         = $(firstword $(subst _, ,$(filter %_tgx,$(subst __, ,$1))))
get-tgy         = $(firstword $(subst _, ,$(filter %_tgy,$(subst __, ,$1))))

TESTS += $(call test-name,16,8,2,1,1,1)
TESTS += $(call test-name,1024,1024,32,4,8,4)
TESTS += $(call test-name,1024,1024,32,32,2,2)
TESTS += $(call test-name,1024,1024,32,128,1,1)

define get-test-parameters
$(eval ROWS=$(call get-rows,$1))
$(eval COLS=$(call get-cols,$1))
$(eval NNZ_PER_ROW=$(call get-nnz-per-row,$1))
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
@echo ROWS=$(ROWS) >> $2
@echo COLS=$(COLS) >> $2
@echo NNZ_PER_ROW=$(NNZ_PER_ROW) >> $2
@echo GROUPS=$(GROUPS) >> $2
@echo TGX=$(TGX) >> $2
@echo TGY=$(TGY) >> $2
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmv

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk
