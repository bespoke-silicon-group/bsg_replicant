# This makefile fragment defines the rules that are re-used between cosimulation
# sub-suites.

# The following rules generate the <test_name>.log file by running the
# <test_name> executable. The output .vpd file will be named <test_name>.vpd.
INDEPENDENT_LOGS=$(foreach tgt, $(INDEPENDENT_TESTS), $(EXEC_PATH)/$(tgt).log)
$(INDEPENDENT_LOGS): $(EXEC_PATH)/%.log: $(EXEC_PATH)/% %.rule
	$< 2>&1 +ntb_random_seed_automatic +vpdfile+$<.vpd $(SIM_ARGS)\
		+c_args="$(C_ARGS)" | tee $@

# compilation.mk defines all of the rules for building cosimulation binaries
# It defines the target <test_name> for each regression test.
include $(TESTBENCH_PATH)/compilation.mk

# regression.mk defines the targets regression and <test_name.log>
include $(REGRESSION_PATH)/regression.mk

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<test_name>|<test_name>.log}"
	@echo "      regression: Run all cosimulation regression tests for "
	@echo "             this subdirectory"
	@echo "      <test_name>: Build the cosimulation binary for a specific"
	@echo "             test"
	@echo "      <test_name.log>: Run a specific cosimulation test and "
	@echo "             generate the log file"
	@echo "      clean: Remove all subdirectory-specific outputs"

clean: regression.clean compilation.clean $(USER_CLEAN_RULES)
	rm -rf vanilla.log vcache_stats.log vanilla_stats.log

