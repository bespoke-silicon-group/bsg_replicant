# Copyright (c) 2019, University of Washington All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
# 
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
# 
# Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This Makefile fragment defines the rules that are re-used between cosimulation
# sub-suites.

# REGRESSION_PATH: The path to the regression folder in BSG F1
ifndef REGRESSION_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_PATH is not defined$(NC)"))
endif

# TESTBENCH_PATH: The path to the regression folder in BSG F1
ifndef TESTBENCH_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: TESTBENCH_PATH is not defined$(NC)"))
endif

# REGRESSION_TESTS: The path to the regression folder in BSG F1
ifndef REGRESSION_TESTS
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_TESTS is not defined$(NC)"))
endif

# EXEC_PATH: The path to the regression folder in BSG F1
ifndef EXEC_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: EXEC_PATH is not defined$(NC)"))
endif

# The bsg_manycore_runtime headers are in $(LIBRARIES_PATH) (for cosimulation)
INCLUDES   += -I$(LIBRARIES_PATH) 

# CSOURCES/HEADERS should probably go in some regression file list.
CDEFINES   += -DCOSIM
CXXDEFINES += -DCOSIM
CXXFLAGS   += -lstdc++

include $(REGRESSION_PATH)/compilation.mk

# link.mk defines all of the linker targets for building executable
# binaries. It defines the target <test_name> for each regression
# test.
include $(TESTBENCH_PATH)/link.mk

# regression.mk defines the targets regression and <test_name.log>
include $(REGRESSION_PATH)/regression.mk

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {target(s)}"
	@echo "      regression: Run all cosimulation regression tests for "
	@echo "             this subdirectory and print pass/fail results"
	@echo ""
	@echo "      regression+traces: Run all cosimulation regression tests"
	@echo "             for this subdirectory and generate traces for each"
	@echo "             test. Does not print aggregate pass/fail results."
	@echo ""
	@echo "      <test_name>: Build the cosimulation binary for a specific"
	@echo "             test"
	@echo ""
	@echo "      <test_name>.log: Run a specific cosimulation test and "
	@echo "             generate the log file (will not generate traces)"
	@echo ""
	@echo "      <test_name>.vanilla.log: Run a specific cosimulation test"
	@echo "             and generate the vanilla.log file."
	@echo ""
	@echo "      <test_name>.vanilla_operation_trace.csv: Run a specific"
	@echo "             cosimulation test and generate the"
	@echo "             vanilla_operation_trace.csv file."
	@echo ""
	@echo "      clean: Remove all subdirectory-specific outputs"
	@echo ""
	@echo "      help: Print this message (default)"
	@echo ""
	@echo "Available <test_name> targets:"
	@echo "$(REGRESSION_TESTS)" | fold -s -w70 | sed -e "s|^ ||g;s|^|\t|g"


# This defines "local" goals for each regression test target
test_loader $(INDEPENDENT_TESTS): %: $(EXEC_PATH)/%

# Normal cosimulation requires a simulation binary without waveform flags (it
# executes faster...). We call this binary <test_name>, and its build rule is
# defined in link.mk.
#
# UNIFIED_TESTS all use test_loader. INDEPENDENT_TESTS use their own top-level
# that corresponds to their <test_name>. Encode dependencies for each
# differently.
#
# The ORDER of these dependencies are important because the actual rule uses $<
# to get the name of the executable.
$(UNIFIED_TESTS:%=.%/vanilla_stats.csv): $(EXEC_PATH)/test_loader
$(INDEPENDENT_TESTS:%=.%/vanilla_stats.csv): .%/vanilla_stats.csv : $(EXEC_PATH)/%
# Each regression test can define its dependencies in <test_name>.rule, but we
# must satisfy that rule before generating the .csv file
$(REGRESSION_TESTS:%=.%/vanilla_stats.csv): .%/vanilla_stats.csv : %.rule

# These are the execution rules for the binaries. We define TEST_NAME
# so that it can be used in C_ARGS, and LOG_NAME so that we can write
# a log, <test_name>.log. Finally, depend on <test_name>.rule so that
# we rebuild the RISC-V binaries.

.%/vanilla_stats.csv: TEST_NAME=$(@:.%/$(notdir $@)=%)
.%/vanilla_stats.csv: LOG_NAME=$(subst ..,.,$(TEST_NAME).log)

.%/vanilla_stats.csv:
	mkdir -p $(dir $@)
	cd $(dir $@) && \
	$< $(SIM_ARGS) $(C_ARGS) 2>&1 | tee $(LOG_NAME)
	@mv $(dir $@)/$(LOG_NAME) $(LOG_NAME)


# set the the name for MEMSYS_STATS
ifneq ($(filter e_vcache_%, $(CL_MANYCORE_MEM_CFG)),)
MEMSYS_STATS := vcache_stats
endif

ifneq ($(filter e_infinite_mem, $(CL_MANYCORE_MEM_CFG)),)
MEMSYS_STATS := infinite_mem_stats
endif

# Vcache stats are generated by default, at the same time as
# vanilla_stats.
.%/$(MEMSYS_STATS).csv: .%/vanilla_stats.csv ;
%.$(MEMSYS_STATS).csv: .%/$(MEMSYS_STATS).csv
	touch $@

# $(EXEC_PATH)/%.log is an alias for %.vanilla_stats.csv. 
$(EXEC_PATH)/%.log: %.vanilla_stats.csv ;
# vanilla_stats.csv is generated in .<test_name>, so we move it out and into a
# name-specific file.
%.vanilla_stats.csv: .%/vanilla_stats.csv %.$(MEMSYS_STATS).csv
	touch $@

# .%/vanilla_operation_trace.csv, .%/vcache_operation_trace.csv 
# and .%/vanilla.log are generated by running a simulation
# with the +trace flag. They are generated into the .<test_name>
# directory, so we copy them out into %.vanilla.log and
# %.vanilla_operation_trace.csv to make them test-specific.
.%/operation_trace.csv .%/vanilla_operation_trace.csv .%/vcache_operation_trace.csv .%/vanilla.log: SIM_ARGS += +trace
.%/operation_trace.csv .%/vanilla_operation_trace.csv .%/vcache_operation_trace.csv .%/vanilla.log: %.vanilla_stats.csv ;

%.vanilla_operation_trace.csv: .%/vanilla_operation_trace.csv
	@mv $< $@

%.vcache_operation_trace.csv: .%/vcache_operation_trace.csv
	@mv $< $@

# %.operation_trace.csv generates both vanilla_operation_trace.csv
# and vcache_operation_trace.csv
%.operation_trace.csv: %.vanilla_operation_trace.csv %.vcache_operation_trace.csv ;

%.vanilla.log: .%/vanilla.log
	@mv $< $@

clean: regression.clean compilation.clean link.clean $(USER_CLEAN_RULES)
	rm -rf *.log
	rm -rf *.vanilla_operation_trace.csv *.vcache_operation_trace.csv *.vanilla_stats.csv
	rm -rf *.vcache_stats.csv *.infinite_mem_stats.csv
	rm -rf $(MACHINES_PATH)/*/synopsys_sim.setup
	rm -rf $(MACHINES_PATH)/*/regression.log
	rm -rf $(REGRESSION_TESTS) test_loader
	rm -rf $(REGRESSION_TESTS:%=.%)


.PHONY: %.dve
.PRECIOUS: %.csv %.vanilla_stats.csv %.log %.$(MEMSYS_STATS).csv
