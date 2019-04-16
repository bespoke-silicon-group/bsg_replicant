include ../Makefile.environment
.DEFAULT_GOAL=cosim
TARGETS=cosim

.PHONY: $(TARGETS) clean $(CLEAN_TARGETS)

$(TARGETS):
	make -C $@ $(if $(DEBUG),DEBUG=$(DEBUG))

CLEAN_TARGETS := $(addsuffix .clean,$(TARGETS))
clean: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) -C $(basename $@) clean
