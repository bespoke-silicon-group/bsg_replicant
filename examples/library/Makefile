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
REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk

# Defines REGRESSION_PREBUILD
include $(EXAMPLES_PATH)/link.mk

# Define the tests that get run
TESTS += test_rom
TESTS += test_coordinate
TESTS += test_get_cycle
TESTS += test_struct_size
TESTS += test_vcache_flush
TESTS += test_vcache_simplified
TESTS += test_vcache_stride
TESTS += test_vcache_sequence
TESTS += test_printing
TESTS += test_manycore_alignment
TESTS += test_manycore_packets
TESTS += test_manycore_init
TESTS += test_manycore_dmem_read_write
TESTS += test_manycore_vcache_sequence
TESTS += test_manycore_dram_read_write
TESTS += test_manycore_credits
TESTS += test_manycore_eva_read_write
TESTS += test_read_mem_scatter_gather
#TESTS += test_packet
TESTS += test_pod_iteration

regression: $(TESTS) ;
$(TESTS): $(REGRESSION_PREBUILD)
	$(MAKE) -C $@ regression

.PHONY: clean regression $(TESTS)

clean: $(TESTS:=.clean)

%.clean:
	$(MAKE) -C $(@:.clean=) clean
