# This makefile fragment defines the rules that are re-used between execution
# regression sub-suites.

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 
# REGRESSION_TESTS: Names of all available regression tests.
ifndef REGRESSION_TESTS
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_TESTS is not defined$(NC)"))
endif

# REGRESSION_PATH: The path to the regression folder in BSG F1
ifndef REGRESSION_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_PATH is not defined$(NC)"))
endif

INDEPENDENT_LOGS=$(foreach tgt, $(INDEPENDENT_TESTS), $(EXEC_PATH)/$(tgt).log)
$(INDEPENDENT_LOGS): $(EXEC_PATH)/%.log: $(EXEC_PATH)/% %.rule
	sudo $< | tee $@

# compillation.mk defines rules for compilation of the C/C++ regression tests.
# It defines the target <test_name> for each regression test.
include $(REGRESSION_PATH)/compilation.mk

# regression.mk defines the targets regression and <test_name.log>
include $(REGRESSION_PATH)/regression.mk

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<test_name>|<test_name>.log}"
	@echo "      regression: Run all regression tests for this subdirectory"
	@echo "      <test_name>: Build regression binary for a specific test"
	@echo "      <test_name.log>: Run a specific regression test and "
	@echo "             generate the log file"
	@echo "      clean: Remove all subdirectory-specific outputs"

.PHONY: help
clean: compilation.clean regression.clean
