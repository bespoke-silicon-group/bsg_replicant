include ../Makefile.environment 
# $(HARDWARE_PATH)/Makefile.include defines $(HARDWARE_TARGETS) which is a list
# of rules that generate hardware for cosimulation and computation. It also defines
# $(HARDWARE_CLEANS), a list of clean rules for cleaning said generated hardware
include $(HARDWARE_PATH)/Makefile.include

TARGETS = library spmd cuda python

DEBUG ?= 0
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?= 0
TURBO ?= 1 # Use if you don't want debug information
EXTRA_TURBO ?= 0 # Use at your own risk.

all: regression
regression: $(TARGETS)

$(TARGETS): $(HARDWARE_TARGETS)
	$(MAKE) -C $@ regression DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) \
		AXI_PROT_CHECK=$(AXI_PROT_CHECK) TURBO=$(TURBO) EXTRA_TURBO=$(EXTRA_TURBO)

.PHONY: clean $(CLEAN_TARGETS) cosimulation $(TARGETS)

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS) $(HARDWARE_CLEANS)
	rm -rf *.log *.jou
	rm -rf .cxl*
	rm -rf *.bak

$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean

squeakyclean: clean
	$(MAKE) -C library squeakyclean
