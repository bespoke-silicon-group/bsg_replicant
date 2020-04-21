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

# This (rather sparse) Makefile is responsible for building and
# running regression tests on F1.  Run `make help` to see the
# available targets. (The target `help` is defined in f1.mk)

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../../environment.mk

# tests.mk defines all of the regression tests (and the source path) for this
# sub-directory. It defines the variable REGRESSION_TESTS_TYPE
include tests.mk

# EXEC_PATH defines this directory, where the tests are actually
# executed. REGRESSION_TESTS_TYPE is defined by tests.mk in each regression
# sub-directory. It is used in f1.mk.
EXEC_PATH=$(REGRESSION_PATH)/$(REGRESSION_TESTS_TYPE)

# f1.mk defines all of the re-used rules for execution on F1. (e.g. regression,
# clean, help, etc)
include $(REGRESSION_PATH)/f1.mk

# rules.mk defines rules for building RISC-V binaries associated with the tests
# in this sub-directory. rules.mk must come after f1.mk so that
# <test_name>.rule and <test_name>.clean are not overwritten.
include rules.mk

# The rule below defines how to run test_loader for Python tests. The loader
# requires one argument: The test name (TEST_NAME)
PYTHON_TEST_PATH = $(BSG_F1_DIR)/regression/pytorch/
$(EXEC_PATH)/%.log: TEST_NAME=$(subst .log,,$(notdir $@))
$(EXEC_PATH)/%.log: $(EXEC_PATH)/test_loader %.rule
	sudo $< $(PYTHON_TEST_PATH)/$(TEST_NAME).py | tee $@
