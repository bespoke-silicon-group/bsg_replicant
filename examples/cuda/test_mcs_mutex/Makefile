REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_lock-type__$(2)_threads__$(3)_critical-region-length__$(4)_non-critical-region-length__$(5)_iters
get-lock-type	= $(firstword $(subst _, ,$(filter %_lock-type,$(subst __, ,$1))))
get-threads     = $(firstword $(subst _, ,$(filter %_threads,$(subst __, ,$1))))
get-iters       = $(firstword $(subst _, ,$(filter %_iters,$(subst __, ,$1))))
get-critical-region-length = $(firstword $(subst _, ,$(filter %_critical-region-length,$(subst __, ,$1))))
get-non-critical-region-length = $(firstword $(subst _, ,$(filter %_non-critical-region-length,$(subst __, ,$1))))

ITERATIONS = 4
THREADS    = 1 2 4 8 16 32 64 128
CRL        = 1
NCRL       = 1
LOCK_TYPE  = mcs

$(foreach type,$(LOCK_TYPE),\
$(foreach threads,$(THREADS),\
$(foreach crl,$(CRL),\
$(foreach ncrl,$(NCRL),\
$(foreach iter,$(ITERATIONS),\
$(eval TESTS += $(call test-name,$(type),$(threads),$(crl),$(ncrl),$(iter))))))))

define get-test-parameters
$(eval THREADS=$(call get-threads,$1))
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
@echo THREADS=$(THREADS)	>> $2
@echo CRL=$(CRL)		>> $2
@echo NCRL=$(NCRL)		>> $2
@echo ITERS=$(ITERS)		>> $2
@echo LOCK_TYPE=$(LOCK_TYPE)	>> $2
endef

# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/test_mcs_mutex

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk
