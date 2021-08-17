REPLICANT_PATH=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

test-name       = $(1)_rows__$(2)_cols__$(3)_nnz-per-row
get-rows        = $(firstword $(subst _, ,$(filter %_rows,$(subst __, ,$1))))
get-cols        = $(firstword $(subst _, ,$(filter %_cols,$(subst __, ,$1))))
get-nnz-per-row = $(firstword $(subst _, ,$(filter %_nnz,$(subst __, ,$1))))

TESTS += $(call test-name,16,16,2)

TESTS_PARAMETERS_MK=$(addsuffix /parameters.mk,$(TESTS))
TESTS_MAKEFILE=$(addsuffix /Makefile,$(TESTS))

define get-test-parameter
$(eval ROWS=$(call get-rows,$1))
$(eval COLS=$(call get-cols,$1))
$(eval NNZ_PER_ROW=$(call get-nnz-per-row,$1))
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
	@echo BSG_MANYCORE_KERNELS=$(EXAMPLES_PATH)/cuda/dwarfs/sparse_linear_algebra/spmv/$* >> $@

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
