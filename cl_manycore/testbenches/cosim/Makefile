TARGETS = library spmd

COSIMULATION_TARGETS := $(addsuffix .cosim,$(TARGETS))
cosim: $(COSIMULATION_TARGETS)
$(COSIMULATION_TARGETS):
	$(MAKE) -C $(basename $@) cosim

REGRESSION_TARGETS := $(addsuffix .regression,$(TARGETS))
regression: $(COSIMULATION_TARGETS) $(REGRESSION_TARGETS)
$(REGRESSION_TARGETS):
	$(MAKE) -C $(basename $@) regression

.PHONY: clean $(CLEAN_TARGETS) cosimulation $(COSIMULATION_TARGETS) 

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean
