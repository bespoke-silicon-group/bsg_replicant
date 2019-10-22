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
	rm -rf vanilla.log vcache_stats.log vanilla_stats.log vanilla_stall_trace.log
