TARGETS = library spmd cuda python

DEBUG ?= 0
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?= 0
TURBO ?= 1 # Use if you don't want debug information
EXTRA_TURBO ?= 0 # Use at your own risk.

all: regression
regression: $(TARGETS)

$(TARGETS): 
	$(MAKE) -C $@ regression DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) \
		AXI_PROT_CHECK=$(AXI_PROT_CHECK) TURBO=$(TURBO) EXTRA_TURBO=$(EXTRA_TURBO)

.PHONY: clean $(CLEAN_TARGETS) cosimulation $(TARGETS)

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS) 
	rm -rf *.log *.jou
	rm -rf .cxl*
	rm -rf *.bak

$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean

squeakyclean: clean
	$(MAKE) -C library squeakyclean
