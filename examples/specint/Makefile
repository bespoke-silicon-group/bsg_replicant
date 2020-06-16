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
# available targets. (The target `help` is defined in compilation.mk

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# LIBRAIRES_PATH: The path to the libraries directory
# HARDWARE_PATH: The path to the hardware directory
# EXAMPLES_PATH: The path to the examples directory
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../../environment.mk

# tests.mk defines all of the regression tests (and the source path)
# for this sub-directory.
include tests.mk

# compillation.mk defines rules for compilation of the C/C++
# regression tests.
include $(REGRESSION_PATH)/compilation.mk

# rules.mk defines rules for building RISC-V binaries associated with
# the tests in this sub-directory. rules.mk must come after
# cosimulation.mk so that <test_name>.rule and <test_name>.clean are
# not overwritten.
include rules.mk

# The rule below defines how to run test_loader for SPECINT tests. The loader
# requires two arguments: The path to the RISC-V Binary loaded onto the manycore
# (TEST_PATH), and the test name (TEST_NAME). SPECINT_PATH is defined for convenience
SPECINT_PATH=$(BSG_MANYCORE_DIR)/software/spmd/specint2000
$(EXEC_PATH)/%.log: TEST_NAME=$(subst .log,,$(notdir $@))
$(EXEC_PATH)/%.log: TEST_PATH=$(SPECINT_PATH)/$(subst test_,,$(TEST_NAME))/main.riscv
$(EXEC_PATH)/%.log: #$(EXEC_PATH)/test_loader #%.rule
	sudo $< $(TEST_PATH) $(TEST_NAME) | tee $@


