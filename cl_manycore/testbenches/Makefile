include ../Makefile.environment
.DEFAULT_GOAL=cosim
TARGETS=cosim

.PHONY: $(TARGETS) clean $(CLEAN_TARGETS)

$(TARGETS):
	make -C $@ DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) AXI_PROT_CHECK=$(AXI_PROT_CHECK)

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean
