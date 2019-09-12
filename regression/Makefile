# This Makefile is for running cosimulation regression
#
# targets.mk defines the targets (sub-directories) for regression in
# F1
include targets.mk

.PHONY: help clean %.clean regression $(TARGETS)

clean: $(addsuffix .clean,$(TARGETS))
%.clean:
	$(MAKE) -C $(basename $@) clean

regression: $(TARGETS)
$(TARGETS): 
	$(MAKE) -C $@ regression

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<subdirectory_name>}"
	@echo "      regression: Run all F1 regression tests"
	@echo "      <subdirectory_name>: Run all the regression tests for"
	@echo "             a specific sub-directory (Options are: $(TARGETS))"
	@echo "      clean: Remove all build files"
