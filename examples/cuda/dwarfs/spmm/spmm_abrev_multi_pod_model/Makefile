REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name		= $(1)_input__$(2)_partfactor__$(3)_partition__$(4)_opt__$(5)_parallel
get-input		= $(firstword $(subst _, ,$(filter %_input,$(subst __, ,$1))))
get-opt			= $(firstword $(subst _, ,$(filter %_opt,$(subst __, ,$1))))
get-parallel		= $(firstword $(subst _, ,$(filter %_parallel,$(subst __, ,$1))))
get-partfactor		= $(firstword $(subst _, ,$(filter %_partfactor,$(subst __, ,$1))))
get-partition		= $(firstword $(subst _, ,$(filter %_partition,$(subst __, ,$1))))


# This function needs to be defined
APPLICATION_PATH=$(EXAMPLES_PATH)/cuda/dwarfs/spmm

# full thing
include $(APPLICATION_PATH)/utils.mk
PFACTOR=8
PARTITIONS=$(call cartesian,$(call range,0,$(PFACTOR)),$(call range,0,$(PFACTOR)),x)

$(foreach part,$(PARTITIONS),\
$(eval TESTS += $(call test-name,offshore,$(PFACTOR),$(part),yes,yes)))
#$(eval TESTS += $(call test-name,wiki-vote,$(PFACTOR),$(part),yes,yes)))
#$(eval TESTS += $(call test-name,road-central,$(PFACTOR),$(part),yes,yes)))
#$(eval TESTS += $(call test-name,offshore,16,$(part),yes,yes)))
#$(eval TESTS += $(call test-name,offshore,16,$(part),yes,yes)))

#$(eval TESTS += $(call test-name,roadNet-CA,$(PFACTOR),$(part),yes,yes)))
# $(eval TESTS += $(call test-name,road-central,$(PFACTOR),$(part),yes,yes))\
# $(eval TESTS += $(call test-name,hollywood,$(PFACTOR),$(part),yes,yes))\
# $(eval TESTS += $(call test-name,LiveJournal,$(PFACTOR),$(part),yes,yes))\
# $(eval TESTS += $(call test-name,soc-Pokec,$(PFACTOR),$(part),yes,yes))\
# $(eval TESTS += $(call test-name,road-usa,$(PFACTOR),$(part),yes,yes)))

define get-test-parameters
$(eval INPUT=$(call get-input,$1))
$(eval ROW_BASE=$(call get-row-base,$1))
$(eval ROWS=$(call get-rows,$1))
$(eval OPT=$(call get-opt,$1))
$(eval PARALLEL=$(call get-parallel,$1))
$(eval PARTFACTOR=$(call get-partfactor,$1))
$(eval PARTITION=$(call get-partition,$1))
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
@echo ROW_BASE=$(ROW_BASE)	>> $2
@echo ROWS=$(ROWS)		>> $2
@echo OPT=$(OPT)		>> $2
@echo PARALLEL=$(PARALLEL)	>> $2
@echo PARTITION=$(PARTITION)    >> $2
@echo PARTFACTOR=$(PARTFACTOR)  >> $2
endef

# This can be overriden to set a custom simulation directory for a test.
# Defaults to $(APPLICATION_PATH)/$(test-name)
#
# See spmm for examples of overriding
define get-sim-dir-from-test
$(eval SIMULATION_DIR=$(APPLICATION_PATH)/spmm_abrev_multi_pod_model/$1)
endef

include $(EXAMPLES_PATH)/cuda/dwarfs/dwarf.mk

$(foreach test,$(TESTS),$(test).stats): %.stats:
	$(MAKE) -C $* stats

stats: $(foreach test,$(TESTS),$(test).stats)
