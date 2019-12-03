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

# This makefile fragment defines the rules that are re-used between cosimulation
# sub-suites.

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
	@echo "      <test_name.vanilla.log>: Run a specific cosimulation test and "
	@echo "             generate the vanilla log file"
	@echo "      clean: Remove all subdirectory-specific outputs"

VANILLA_LOG_RULES = $(addsuffix .vanilla.log,$(REGRESSION_TESTS))
$(VANILLA_LOG_RULES): SIM_ARGS +=+trace
$(VANILLA_LOG_RULES): %.vanilla.log: $(EXEC_PATH)/%.log
	@mv vanilla.log $@
	@mv vcache_non_blocking_stats.log $(subst vanilla,vcache_non_blocking_stats,$@)
	@mv vanilla_operation_trace.csv $(subst vanilla.log,vanilla_operation_trace.csv,$@)
	@mv vanilla_stats.csv $(subst vanilla.log,vanilla_stats.csv,$@)

clean: regression.clean compilation.clean $(USER_CLEAN_RULES)
	rm -rf *.log
	rm -rf *.vanilla_operation_trace.csv *.vanilla_stats.csv
