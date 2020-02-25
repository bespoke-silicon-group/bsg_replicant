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
INCLUDES   += -I$(VCS_HOME)/linux64/lib/

# CSOURCES/HEADERS should probably go in some regression file list.
CDEFINES   += -DCOSIM -DVCS
CXXDEFINES += -DCOSIM -DVCS
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
	@echo "make {regression|clean|<test_name>|<test_name>.log}"
	@echo "      regression: Run all cosimulation regression tests for "
	@echo "             this subdirectory"
	@echo "      waveforms: Run all cosimulation regression tests for "
	@echo "             this subdirectory and generate waveforms"
	@echo "      traces: Run all cosimulation regression tests for "
	@echo "             this subdirectory and generate traces"
	@echo "      <test_name>: Build the cosimulation binary for a specific"
	@echo "             test"
	@echo "      <test_name.log>: Run a specific cosimulation test and "
	@echo "             generate the log file (will not generate traces)"
	@echo "      <test_name.vanilla.log>: Run a specific cosimulation test and "
	@echo "             generate the vanilla.log and vanilla_operation_trace.csv "
	@echo "             and vanilla_operation_trace.csv file"
	@echo "      <test_name.vanilla_operation_trace.csv>: Same as above"
	@echo ""
	@echo "      clean: Remove all subdirectory-specific outputs"

waveforms: $(REGRESSION_TESTS:%=%.vpd)
traces: $(REGRESSION_TESTS:%=%.vanilla.log)
test_loader $(REGRESSION_TESTS): %: $(EXEC_PATH)/%

$(EXEC_PATH)/%.log: %.vanilla_stats.csv ;

SIM_ARGS += +rad +ntb_random_seed_automatic 
$(EXEC_PATH)/%.vpd: SIM_ARGS += +vcs+vcdpluson +vcs+vcdplusmemon +memcbk
$(EXEC_PATH)/%.vpd: SIM_ARGS += +vpdfile+$@

$(UNIFIED_TESTS:%=%.vpd): $(EXEC_PATH)/test_loader.debug
$(INDEPENDENT_TESTS:%=%.vpd): %.vpd : $(EXEC_PATH)/%.debug
$(REGRESSION_TESTS:%=%.vpd): %.vpd : %.rule

$(UNIFIED_TESTS:%=%.vanilla_stats.csv): $(EXEC_PATH)/test_loader
$(INDEPENDENT_TESTS:%=%.vanilla_stats.csv): %.vanilla_stats.csv : $(EXEC_PATH)/%
$(REGRESSION_TESTS:%=%.vanilla_stats.csv): %.vanilla_stats.csv : %.rule

%.vanilla_stats.csv %.vpd:
	mkdir -p .$(TEST_NAME)
	cd .$(TEST_NAME) && \
	$< 2>&1 $(SIM_ARGS) +c_args="$(C_ARGS)" | tee $(EXEC_PATH)/$(TEST_NAME).log
	@mv .$(TEST_NAME)/vanilla_stats.csv $(TEST_NAME).vanilla_stats.csv

%.vanilla_operation_trace.csv %.vanilla.log: TEST_NAME = $(basename $(basename $(notdir $@)))
%.vanilla_operation_trace.csv %.vanilla.log: SIM_ARGS += +trace
%.vanilla_operation_trace.csv %.vanilla.log: $(EXEC_PATH)/%.log
	@mv $(dir $@)/.$(TEST_NAME)/vanilla.log $(TEST_NAME).vanilla.log
	@mv $(dir $@)/.$(TEST_NAME)/vanilla_operation_trace.csv $(TEST_NAME).vanilla_operation_trace.csv


%.dve: %.vpd
	$(DVE) -full64 -vpd $< &

clean: regression.clean compilation.clean link.clean $(USER_CLEAN_RULES)
	rm -rf *.log
	rm -rf *.vanilla_operation_trace.csv *.vanilla_stats.csv
	rm -rf $(REGRESSION_TESTS) test_loader 
	rm -rf $(REGRESSION_TESTS:%=%.debug) test_loader.debug
	rm -rf $(REGRESSION_TESTS:%=.%)


.PHONY: %.dve
