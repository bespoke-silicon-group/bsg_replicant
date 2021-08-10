REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

# $1 = table size
# $2 = loads per core
# $3 = ilp
# $4 = stride words
# $5 = tile-x
# $6 = tile-y

test-name = $(1)_table-words__$(2)_loads-per-core__$(3)_ilp__$(4)_stride-words__$(5)_tile-x__$(6)_tile-y

# $1 = test name
get-table-words = $(firstword $(subst _, ,$(filter %_table-words,$(subst __, ,$1))))

# $1 = test name
get-loads-per-core = $(firstword $(subst _, ,$(filter %_loads-per-core,$(subst __, ,$1))))

# $1 = test name
get-ilp = $(firstword $(subst _, ,$(filter %_ilp,$(subst __, ,$1))))

# $1 = test name
get-stride-words = $(firstword $(subst _, ,$(filter %_stride-words,$(subst __, ,$1))))

# $1 = test name
get-tile-x = $(firstword $(subst _, ,$(filter %_tile-x,$(subst __, ,$1))))

# $1 = test name
get-tile-y = $(firstword $(subst _, ,$(filter %_tile-y,$(subst __, ,$1))))

define get-test-parameters
$(eval TABLE_WORDS=$(call get-table-words,$1))
$(eval LOADS_PER_CORE=$(call get-loads-per-core,$1))
$(eval ILP=$(call get-ilp,$1))
$(eval STRIDE_WORDS=$(call get-stride-words,$1))
$(eval TILE_X=$(call get-tile-x,$1))
$(eval TILE_Y=$(call get-tile-y,$1))
endef

TESTS               =$(call test-name,8192,1024,1,1,0,0)
TESTS_MAKEFILE      =$(foreach test,$(TESTS),$(test)/Makefile)
TESTS_PARAMETERS_MK =$(foreach test,$(TESTS),$(test)/parameters.mk)

do_debug:
	@echo $(TESTS_PARAMETERS_MK)
	@echo $(TESTS_MAKEFILE)

$(TESTS_MAKEFILE): %/Makefile: template.mk
	@echo "Creating $@"
	@mkdir -p $*
	@cp $< $@

$(TESTS_PARAMETERS_MK): %/parameters.mk: %/Makefile
	@echo "Creating $@"
	@mkdir -p $*
	@$(eval $(call get-test-parameters,$*))
	@touch $@
	@echo "TABLE_WORDS=$(TABLE_WORDS)"		>> $@
	@echo "LOADS_PER_CORE=$(LOADS_PER_CORE)"	>> $@
	@echo "ILP=$(ILP)"				>> $@
	@echo "STRIDE_WORDS=$(STRIDE_WORDS)"            >> $@
	@echo "TILE_X=$(TILE_X)"                        >> $@
	@echo "TILE_Y=$(TILE_Y)"                        >> $@
	@echo "RISCV_PATH=$(EXAMPLES_PATH)/cuda/stride/$*/kernel.riscv" >> $@

$(TESTS): %: %/Makefile %/parameters.mk

.PHONY: all purge

# $1 = $(test-name).$(run-type)
define get-test-dir-and-run-type
endef

TESTS_PROFILE	= $(foreach test,$(TESTS),$(test).profile)
TESTS_EXEC	= $(foreach test,$(TESTS),$(test).exec)
TESTS_DEBUG	= $(foreach test,$(TESTS),$(test).debug)
TESTS_SAIFGEN   = $(foreach test,$(TESTS),$(test).saifgen)

$(TESTS_PROFILE): %.profile:  %
$(TESTS_EXEC):    %.exec:     %
$(TESTS_DEBUG):   %.debug:    %
$(TESTS_SAIFGEN): %.saifgen:  %
$(TESTS_PROFILE) $(TESTS_EXEC) $(TESTS_DEBUG) $(TESTS_SAIFGEN):
	@$(eval RUN_DIR=$(firstword $(subst ., ,$@)))
	@$(eval RUN_TYPE=$(lastword $(subst ., ,$@)))
	$(MAKE) -C $(RUN_DIR) $(RUN_TYPE).log

profile: $(TESTS_PROFILE)
exec:    $(TESTS_EXEC)
debug:   $(TESTS_DEBUG)
saifgen: $(TESTS_SAIFGEN)
tests:   $(TESTS)

all: profile exec debug saifgen



purge:
	rm -rf $(TESTS)
