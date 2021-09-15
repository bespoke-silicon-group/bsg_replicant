REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_input__$(2)_tx__$(3)_ty
get-input       = $(firstword $(subst _, ,$(filter %_input,$(subst __, ,$1))))
get-tx		= $(firstword $(subst _, ,$(filter %_tx,$(subst __, ,$1))))
get-ty		= $(firstword $(subst _, ,$(filter %_ty,$(subst __, ,$1))))

TESTS += $(call test-name,08blocks,1,1)

define get-test-parameters
$(eval INPUT=$(call get-input,$1))
$(eval TX=$(call get-tx,$1))
$(eval TY=$(call get-ty,$1))
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
@echo INPUT=$(INPUT) >> $2
@echo TX=$(TX) >> $2
@echo TY=$(TY) >> $2
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk
include $(APPLICATION_PATH)/inputs.mk

plots: $(addsuffix .plots,$(TESTS))
$(addsuffix .plots,$(TESTS)): %.plots: %
	$(MAKE) -C $* plots

purge: clean.inputs
