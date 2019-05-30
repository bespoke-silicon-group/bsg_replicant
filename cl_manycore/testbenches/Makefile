include ../Makefile.environment

TARGETS = library spmd

all: cosim
cosim: $(TARGETS)

$(TARGETS):
	$(MAKE) -C $@ regression DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) AXI_PROT_CHECK=$(AXI_PROT_CHECK)

.PHONY: clean $(CLEAN_TARGETS) cosimulation $(TARGETS)

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean
