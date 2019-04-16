TARGETS = library spmd

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean

REGRESSION_TARGETS := $(addsuffix .regression,$(TARGETS))
regression: $(REGRESSION_TARGETS)
$(REGRESSION_TARGETS):
	$(MAKE) -C $(basename $@) regression

.PHONY: clean $(CLEAN_TARGETS) regression $(REGRESSION_TARGETS) 
