REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)
include $(REPLICANT_PATH)/environment.mk

# subrepositories to checkout
repos := hammerblade-helpers graph-tools
hammerblade-helpers.url    = git@github.com:mrutt92/hammerblade-helpers
hammerblade-helpers.commit = master
graph-tools.url    = git@github.com:mrutt92/graph-tools
graph-tools.commit = master
$(repos): %:
	git clone $($*.url)
	cd $* && git checkout $($*.commit)
	cd $* && git submodule update --init

checkout_repos: $(repos)

# $1 = Table size in words
# $2 = Updates per core
# $3 = Cores to use
# $4 = Concurrency
# $5 = exec|profile|debug|saifgen
test-name = $(1)_table-words__$(2)_updates-per-core__$(3)_cores__$(4)_concurrency__$(5)_run
# $1 = Test name
get-table-words = $(firstword $(subst _, ,$(filter %_table-words,$(subst __, ,$1))))
# $1 = Test name
get-updates-per-core = $(firstword $(subst _, ,$(filter %_updates-per-core,$(subst __, ,$1))))
# $1 = Test name
get-cores = $(firstword $(subst _, ,$(filter %_cores,$(subst __, ,$1))))
# $1 = Test name
get-concurrency = $(firstword $(subst _, ,$(filter %_concurrency,$(subst __, ,$1))))
# $1 = Test name
get-run-type = $(firstword $(subst _, ,$(filter %_run,$(subst __, ,$1))))

TABLE_SIZE_NUMS	        = $(shell echo 256*1024*1024|bc)
UPDATES_PER_CORE_NUMS	= 512
CORES_NUMS		= 1 2 4 8 16 32 64 128
CORES_NUMS             += 36 40 44
CORES_NUMS             += 28 24 20
CONCURRENCY_NUMS	= 1 2 4 8 16

$(foreach tblsz,$(TABLE_SIZE_NUMS),\
$(foreach upc,$(UPDATES_PER_CORE_NUMS),\
$(foreach cores,$(CORES_NUMS),\
$(foreach conc,$(CONCURRENCY_NUMS),\
$(eval TESTS += $(call test-name,$(tblsz),$(upc),$(cores),$(conc),profile))))))

# $1 test-name
define set-submake-args
# parse the experiment parameters from the test name
$(eval TABLE_WORDS=$(call get-table-words,$1))
$(eval UPDATES_PER_CORE=$(call get-updates-per-core,$1))
$(eval CORES=$(call get-cores,$1))
$(eval RUN_TYPE=$(call get-run-type,$1))
$(eval CONCURRENCY=$(call get-concurrency,$1))
# set variables to pass to make
$(eval EXTRA_C_ARGS="$(TABLE_WORDS) $(UPDATES_PER_CORE) $(CORES)")
$(eval EXTRA_RISCV_CCPPFLAGS="-DCONCURRENCY=$(CONCURRENCY)")
$(eval MAKE_ARGS  = EXTRA_C_ARGS=$(EXTRA_C_ARGS))
$(eval MAKE_ARGS += EXTRA_RISCV_CCPPFLAGS=$(EXTRA_RISCV_CCPPFLAGS))
$(eval MAKE_ARGS += BSG_MANYCORE_KERNELS=$(EXAMPLES_PATH)/cuda/gups/$1/main.riscv)
endef

TESTS_DISASSEMBLY = $(addsuffix .dis,$(TESTS))
TESTS_BLOOD_GRAPH = $(addsuffix .blood_graph,$(TESTS))

$(TESTS_BLOOD_GRAPH):
	$(eval t=$(firstword $(subst ., ,$@)))
	$(call set-submake-args,$t)
	mkdir -p $t/
	cp template.mk $t/Makefile
	$(MAKE) -C $t $(MAKE_ARGS) blood_graph

$(TESTS_DISASSEMBLY):
	$(eval t=$(firstword $(subst ., ,$@)))
	$(call set-submake-args,$t)
	mkdir -p $t/
	cp template.mk $t/Makefile
	$(MAKE) -C $t $(MAKE_ARGS) main.dis

.PHONY: $(TESTS)
$(TESTS):
	$(call set-submake-args,$@)
	mkdir -p $@/
	cp template.mk $@/Makefile
	$(MAKE) -C $@ $(MAKE_ARGS) $(RUN_TYPE).log

$(TESTS) $(TESTS_DISASSEMBLY) $(TESTS_BLOOD_GRAPH): checkout_repos

all: $(TESTS)
profile:   $(filter %__profile_run,$(TESTS))
debug:     $(filter %__debug_run,$(TESTS))
exec:      $(filter %__exec_run,$(TESTS))
saifgen:   $(filter %__saifgen_run,$(TESTS))

dis: $(TESTS_DISASSEMBLY)
blood_graph: $(TESTS_BLOOD_GRAPH)

dramsim3.summary.csv:  $(addsuffix /dramsim3.tag.json,$(filter %__profile_run,$(TESTS)))
	@echo Making $@
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/gups/hammerblade-helpers/py python3 $(EXAMPLES_PATH)/cuda/gups/py/dramsim3.py $^

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
	@echo Creating $@
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/gups/hammerblade-helpers/py python3 $(EXAMPLES_PATH)/cuda/gups/py/vcache.py $^

purge:
	rm -rf $(TESTS)
