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

# This Makefile fragment defines the compilation rules that are re-used between execution
# regression sub-suites.

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 

# REGRESSION_PATH: The path to the regression folder in BSG F1
ifndef REGRESSION_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_PATH is not defined$(NC)"))
endif

# compillation.mk defines rules for compilation of the C/C++ regression tests.
include $(REGRESSION_PATH)/compilation.mk

# link.mk defines rules for linking of .o files for regression tests.
# It defines the target <test_name> for each regression test.
include $(REGRESSION_PATH)/link.mk

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
