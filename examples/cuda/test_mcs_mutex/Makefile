REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_lock-type__$(2)_tx__$(3)_ty__$(4)_critical-region-length__$(5)_non-critical-region-length__$(6)_iters
get-lock-type	= $(firstword $(subst _, ,$(filter %_lock-type,$(subst __, ,$1))))
get-tx          = $(firstword $(subst _, ,$(filter %_tx,$(subst __, ,$1))))
get-ty          = $(firstword $(subst _, ,$(filter %_ty,$(subst __, ,$1))))
get-iters       = $(firstword $(subst _, ,$(filter %_iters,$(subst __, ,$1))))
get-critical-region-length = $(firstword $(subst _, ,$(filter %_critical-region-length,$(subst __, ,$1))))
get-non-critical-region-length = $(firstword $(subst _, ,$(filter %_non-critical-region-length,$(subst __, ,$1))))

ITERATIONS = 64
TX	   = 1 2 4 8 16
TY	   = 1 2 4 8
CRL        = 1 10 100 1000
NCRL       = 1
LOCK_TYPE  = mcs simple

$(foreach type,$(LOCK_TYPE),\
$(foreach tx,$(TX),\
$(foreach ty,$(TY),\
$(foreach crl,$(CRL),\
$(foreach ncrl,$(NCRL),\
$(foreach iter,$(ITERATIONS),\
$(eval TESTS += $(call test-name,$(type),$(tx),$(ty),$(crl),$(ncrl),$(iter)))))))))

define get-test-parameters
$(eval TX=$(call get-tx,$1))
$(eval TY=$(call get-ty,$1))
$(eval CRL=$(call get-critical-region-length,$1))
$(eval NCRL=$(call get-non-critical-region-length,$1))
$(eval ITERS=$(call get-iters,$1))
$(eval LOCK_TYPE=$(call get-lock-type,$1))
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
@echo TX=$(TX)			>> $2
@echo TY=$(TY)			>> $2
@echo CRL=$(CRL)		>> $2
@echo NCRL=$(NCRL)		>> $2
@echo ITERS=$(ITERS)		>> $2
@echo LOCK_TYPE=$(LOCK_TYPE)	>> $2
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/test_mcs_mutex

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
vcache.summary.csv: $(APPLICATION_PATH)/py/mutex_common.py
vcache.summary.csv: $(APPLICATION_PATH)/py/vcache.py
	$(eval inputs=$(filter %.csv,$^))
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/dwarfs/imports/hammerblade-helpers/py python3 $(APPLICATION_PATH)/py/vcache.py $(inputs)
