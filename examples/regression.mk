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

ifndef __BSG_REGRESSION_MK
__BSG_REGRESSION_MK := 1

# This Makefile fragment defines the regression rules that are reused between ALL
# regression directories (Cosimulation and F1 Execution).

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this fragment...
# 
# REGRESSION_TESTS: Names of all available regression tests.
ifndef REGRESSION_TESTS
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_TESTS is not defined$(NC)"))
endif

# We build a list of LOG_RULES for the regression rule (below)
LOG_RULES = $(addsuffix .log,$(REGRESSION_TESTS))

# REGRESSION_TESTS. Each test can also define a custom rule, <test_name>.rule
# that is run during compilation. These custom rules are useful for building
# spmd or cuda binaries, for example.
USER_RULES:=$(addsuffix .rule,$(REGRESSION_TESTS))
$(USER_RULES):
USER_CLEAN_RULES=$(addsuffix .clean,$(REGRESSION_TESTS))
$(USER_CLEAN_RULES):

# The regression target runs all of the tests in the REGRESSION_TESTS variable
# for a directory.
regression: regression.log 
regression.log: $(LOG_RULES)
	@pass=0; total=0; \
	echo ""| tee $@; \
	echo "==========================================================="| tee -a $@; \
	echo ""| tee -a $@; \
	echo "Parsing $(REGRESSION_TESTS_TYPE) Regression Test results..."| tee -a $@; \
	echo ""| tee -a $@; \
	echo "==========================================================="| tee -a $@; \
	echo ""| tee -a $@; \
	for target in $(notdir $(basename $^)); do \
		if grep "BSG REGRESSION TEST .*PASSED.*" $$target.log > /dev/null; then \
			echo "PASS: Regression Test $$target passed!"| tee -a $@; \
			let "pass+=1"; \
		else \
			echo "FAIL: Regression Test $$target failed!"| tee -a $@; \
		fi; \
		let "total+=1"; \
	done; \
	if [ ! $$pass == $$total ]; then \
		echo "==================================================="| tee -a $@; \
		echo "" | tee -a $@; \
		echo "FAIL! $$pass out of $$total $(REGRESSION_TESTS_TYPE) regression tests passed"| tee -a $@; \
		echo "" | tee -a $@; \
		echo "==================================================="| tee -a $@; \
		exit 1 | tee -a $@; \
	else \
		echo "==========================================================="| tee -a $@; \
		echo ""| tee -a $@; \
		echo "PASS! All $$total tests passed for $(REGRESSION_TESTS_TYPE)"| tee -a $@; \
		echo ""| tee -a $@; \
		echo "==========================================================="| tee -a $@; \
	fi;

regression.clean:
	rm -rf $(LOG_RULES) regression.log

.PHONY: regression regression.clean

endif
