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

# list of subdirectories that we don't treat as SPMD tests
NOT_SPMD = specint2000
NOT_SPMD += bsg_hetero_nodes
NOT_SPMD += bsg_loader_suite \
NOT_SPMD += go_viral
NOT_SPMD += bsg_remote_congestion
NOT_SPMD += memtest1
NOT_SPMD += deprecated
NOT_SPMD += c++
NOT_SPMD += fft
NOT_SPMD += memtest16
NOT_SPMD += fhello_malloc
NOT_SPMD += striped_vector_sum
NOT_SPMD += striped_hello
NOT_SPMD += coremark-top
NOT_SPMD += deprecated
NOT_SPMD += gather_scatter
NOT_SPMD += gather_scatter_multi
NOT_SPMD += striped_struct_vector
NOT_SPMD += beebs
NOT_SPMD += interrupt_tests
NOT_SPMD += bp_manycore_test
NOT_SPMD += bsg_cuda_lite_runtime
NOT_SPMD += common
NOT_SPMD += energy_loop_test
NOT_SPMD += energy_ubenchmark
# Already run as part of test_saif
NOT_SPMD += saif
# Breaks our flow (but could be used)
NOT_SPMD += bsg_riscv_tests
# Runs forever (but could be used)
NOT_SPMD += barrier_in_interrupt
NOT_SPMD += hw_barrier_16x8_interrupt_test
NOT_SPMD += hw_barrier_context_switch_test
NOT_SPMD += memtest2022
# Skip hb_mc test series
ALL_HB_MC_TESTS := $(shell find $(BSG_MANYCORE_DIR)/software/spmd -maxdepth 1 -type d -name 'hb_mc_*' -printf '%P\n')
NOT_SPMD += $(ALL_HB_MC_TESTS)

SPMD_TESTS:=$(shell find $(BSG_MANYCORE_DIR)/software/spmd -maxdepth 1 -type d -printf '%P\n')
SPMD_TESTS:=$(filter-out $(NOT_SPMD),$(SPMD_TESTS))
# Find the intersection of bsg_manycore spmd tests & current directory spmd tests
# So that the flow will not break down when new tests are added to bsg_manycore
CUR_EXISTING_SPMD:=$(shell find $(CURDIR) -maxdepth 1 -type d -printf '%P\n')
NON_EXISTING_SPMD:=$(filter-out $(CUR_EXISTING_SPMD),$(SPMD_TESTS))
SPMD_TESTS:=$(filter $(CUR_EXISTING_SPMD),$(SPMD_TESTS))

# Defines REGRESSION_PREBUILD
include $(EXAMPLES_PATH)/link.mk

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

# Define the tests that get run
TESTS := test_bsg_dram_loopback_cache
TESTS += test_bsg_loader_suite
TESTS += test_bsg_scalar_print
TESTS += test_symbol_to_eva
TESTS += test_saif
TESTS += $(SPMD_TESTS)

regression: $(TESTS)
	@echo "SPMD REGRESSION PASSED"

# Define the hb_mc tests that get run
NOT_SPMD_HB_MC_TESTS := $(shell find $(BSG_MANYCORE_DIR)/software/spmd -maxdepth 1 -type d -name 'hb_mc_*_sdr_init_*' -printf '%P\n')
ifneq (,$(findstring pod_X4Y2_,$(BSG_MACHINE_PATH)))
  NOT_SPMD_HB_MC_TESTS += $(shell find $(BSG_MANYCORE_DIR)/software/spmd -maxdepth 1 -type d -name 'hb_mc_*_sdr_prbs_full' -printf '%P\n')
endif
ifneq (,$(findstring pod_X4Y4_,$(BSG_MACHINE_PATH)))
  NOT_SPMD_HB_MC_TESTS += $(shell find $(BSG_MANYCORE_DIR)/software/spmd -maxdepth 1 -type d -name 'hb_mc_*_sdr_prbs_half' -printf '%P\n')
endif
SPMD_HB_MC_TESTS := $(filter-out $(NOT_SPMD_HB_MC_TESTS),$(ALL_HB_MC_TESTS))
# Find the intersection of bsg_manycore hb_mc tests & current directory hb_mc tests
CUR_EXISTING_HB_MC_SPMD:=$(shell find $(CURDIR) -maxdepth 1 -type d -name 'hb_mc_*' -printf '%P\n')
SPMD_HB_MC_TESTS:=$(filter $(CUR_EXISTING_HB_MC_SPMD),$(SPMD_HB_MC_TESTS))

regression.hb_mc: $(SPMD_HB_MC_TESTS)
	@echo "SPMD HB_MC REGRESSION PASSED"

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=c99 $(FLAGS)
DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -D_DEFAULT_SOURCE
REGRESSION_PREBUILD +=


$(TESTS): $(REGRESSION_PREBUILD)
	$(MAKE) -C $@ regression
	@echo "Non-existing tests (please add them to current directory as soon as \
	possible, empty means all tests exist): $(NON_EXISTING_SPMD)"

$(SPMD_HB_MC_TESTS):
	$(MAKE) -C $@ regression

.PHONY: clean regression $(TESTS) $(SPMD_HB_MC_TESTS)

clean: $(TESTS:=.clean) hardware.clean platform.clean libraries.clean link.clean

clean.hb_mc: $(SPMD_HB_MC_TESTS:=.clean)

%.clean:
	$(MAKE) -C $(@:.clean=) clean

