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

# This Makefile compiles, links, and executes examples Run `make help`
# to see the available targets for the selected platform.

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

REGRESSION_TESTS_TYPE = python

###############################################################################
# Examples List
#
# UNIFIED_TESTS use a generic top-level, test_unified_main.c. They depend on a
# self-checking Manycore kernel to send a PASS/FAIL packet to the host
#
# INDEPENDENT_TESTS use a per-test top level, <test_name>.c/cpp. They check
# correctness on the host.
# 
# REGRESSION_TESTS merges the two lists
###############################################################################
UNIFIED_TESTS = test_python

INDEPENDENT_TESTS := 

REGRESSION_TESTS = $(UNIFIED_TESTS) $(INDEPENDENT_TESTS)

###############################################################################
# Host code compilation flags and flow
###############################################################################
DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

CDEFINES   += $(DEFINES)
CXXDEFINES += $(DEFINES)

FLAGS     = -g -Wall $(shell python3-config --cflags) -O1 
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++11 $(FLAGS) 
LDFLAGS  += $(shell python3-config --ldflags)

###############################################################################
# Execution Arguments (C_ARGS)
#
# Python tests require one argument: the path to the python file
#
###############################################################################

PYTHON_TEST_PATH = $(EXAMPLES_PATH)/python
TEST_NAME=$(subst .log,,$(notdir $@))
C_ARGS = $(PYTHON_TEST_PATH)/$(TEST_NAME).py

include $(EXAMPLES_PATH)/flow.mk

.PHONY: clean

clean:
	rm -rf $(INDEPENDENT_TESTS) test_loader
