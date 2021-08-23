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

TESTS_PARAMETERS_MK=$(addsuffix /parameters.mk,$(TESTS))
TESTS_MAKEFILE=$(addsuffix /Makefile,$(TESTS))

define get-test-parameters
$(eval ROWS=$(call get-rows,$1))
$(eval COLS=$(call get-cols,$1))
$(eval NNZ_PER_ROW=$(call get-nnz-per-row,$1))
$(eval GROUPS=$(call get-groups,$1))
$(eval TGX=$(call get-tgx,$1))
$(eval TGY=$(call get-tgy,$1))
endef

$(TESTS_MAKEFILE): %/Makefile: template.mk
	@echo "Creating $@"
	@mkdir -p $*
	@cp $< $@

$(TESTS_PARAMETERS_MK): %/parameters.mk:
	@echo "Creating $@"
	@mkdir -p $*
	@$(call get-test-parameters,$*)
	@echo ROWS=$(ROWS) >  $@
	@echo COLS=$(COLS) >> $@
	@echo NNZ_PER_ROW=$(NNZ_PER_ROW) >> $@
	@echo GROUPS=$(GROUPS) >> $@
	@echo TGX=$(TGX) >> $@
	@echo TGY=$(TGY) >> $@
	@echo BSG_MANYCORE_KERNELS=$(EXAMPLES_PATH)/cuda/dwarfs/spmv/$*/kernel.riscv >> $@

$(TESTS): %: %/Makefile %/parameters.mk

TESTS_EXEC    = $(addsuffix .exec,$(TESTS))
TESTS_DEBUG   = $(addsuffix .debug,$(TESTS))
TESTS_PROFILE = $(addsuffix .profile,$(TESTS))
TESTS_SAIFGEN = $(addsuffix .saifgen,$(TESTS))

$(TESTS_EXEC):    %.exec:    %
$(TESTS_DEBUG):   %.debug:   %
$(TESTS_PROFILE): %.profile: %
$(TESTS_SAIFGEN): %.saifgen: %

$(TESTS_EXEC) $(TESTS_DEBUG) $(TESTS_PROFILE) $(TESTS_SAIFGEN):
	$(eval TEST_NAME=$(firstword $(subst ., ,$@)))
	$(eval RUN_TYPE=$(lastword $(subst ., ,$@)))
	$(MAKE) -C $(TEST_NAME) $(RUN_TYPE).log

exec:    $(TESTS_EXEC)
debug:   $(TEST_DEBUG)
saifgen: $(TESTS_SAIFGEN)
profile: $(TESTS_PROFILE)
tests:   $(TESTS)

purge:
	rm -rf $(TESTS)
