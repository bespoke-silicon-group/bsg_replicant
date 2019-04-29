TARGETS = library spmd cuda

COSIMULATION_TARGETS := $(addsuffix .cosim,$(TARGETS))
cosim: $(COSIMULATION_TARGETS)
$(COSIMULATION_TARGETS):
	$(MAKE) -C $(basename $@) cosim DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) AXI_PROT_CHECK=$(AXI_PROT_CHECK)

REGRESSION_TARGETS := $(addsuffix .regression,$(TARGETS))
regression: $(COSIMULATION_TARGETS) $(REGRESSION_TARGETS)
$(REGRESSION_TARGETS):
	$(MAKE) -C $(basename $@) regression

.PHONY: clean $(CLEAN_TARGETS) cosimulation $(COSIMULATION_TARGETS) 

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean
