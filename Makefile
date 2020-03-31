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

# This file contains the top-level rules for interacting with this
# project. The rules are documented in the makefile-default "help"
# command

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include environment.mk

.PHONY: help build regression cosim clean

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {build|regression|cosim|clean}"
	@echo "      build: Runs Vivado and Generates the Design Checkpoint to"
	@echo "             upload to AWS."
	@echo "      regression: Runs all software regression tests on F1"
	@echo "      cosim: Runs all regression tests in C/C++ Co-simulation"
	@echo "             on the machine specified by machine.mk"
	@echo "      multiverse: Runs all regression tests on all machines"
	@echo "             and copies the resulting regression.log to the"
	@echo "             corresponding directory in machines"
	@echo "             (multiverse takes a good long while)"
	@echo "      clean: Remove all build files"

build:
	$(MAKE) -C $@ $@ BSG_MACHINE_PATH=$(MACHINES_PATH)/4x4_blocking_vcache_f1_model 

regression:
	$(MAKE) -C regression $@ 

cosim: 
	$(MAKE) -C testbenches regression

__BSG_MACHINES := $(wildcard machines/*)
__BSG_MACHINES := $(filter-out machines/README.md,$(__BSG_MACHINES))
__BSG_MACHINES := $(filter-out machines/timing_v0_32_16,$(__BSG_MACHINES))
__BSG_MACHINES := $(filter-out machines/timing_v0_64_32,$(__BSG_MACHINES))
# F1 with realistic DRAM Takes forever, so we'll leave it out for now
__BSG_MACHINES := $(filter-out machines/4x4_blocking_vcache_f1_dram,$(__BSG_MACHINES))
multiverse:
	$(foreach m,$(__BSG_MACHINES),$(MAKE) -k -C testbenches regression BSG_MACHINE_PATH=`pwd`/$m && cp testbenches/regression.log $m &&) echo ;

clean:
	$(MAKE) -C testbenches clean 
	$(MAKE) -C build clean 
	$(MAKE) -C regression clean
	$(MAKE) -C hardware clean 

