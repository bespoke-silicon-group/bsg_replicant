TARGETS = library spmd

COSIMULATION_TARGETS := $(addsuffix .regression,$(TARGETS))
cosim: $(COSIMULATION_TARGETS)
$(COSIMULATION_TARGETS):
	$(MAKE) -C $(basename $@) regression

.PHONY: clean $(CLEAN_TARGETS) regression $(COSIMULATION_TARGETS) 

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean
