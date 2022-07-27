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
TESTS += test_binary_load_buffer
TESTS += test_empty_parallel
TESTS += test_multiple_binary_load
TESTS += test_host_memset
TESTS += test_stack_load
TESTS += test_memory_leak
TESTS += test_dram_load_store
TESTS += test_dram_host_allocated
TESTS += test_dram_device_allocated
TESTS += test_device_memset
TESTS += test_device_memcpy
TESTS += test_vec_add
#TESTS += test_vec_add_dma
#TESTS += test_dma
TESTS += test_vec_add_parallel
TESTS += test_vec_add_parallel_multi_grid
TESTS += test_vec_add_serial_multi_grid
TESTS += test_vec_add_shared_mem
TESTS += test_max_pool2d
TESTS += test_shared_mem
TESTS += test_shared_mem_load_store
TESTS += test_matrix_mul
TESTS += test_matrix_mul_shared_mem
#TESTS += test_high_mem
TESTS += test_float_all_ops
TESTS += test_float_vec_add
TESTS += test_float_vec_add_shared_mem
TESTS += test_float_vec_mul
TESTS += test_float_vec_div
TESTS += test_float_vec_exp
TESTS += test_float_vec_sqrt
TESTS += test_float_vec_log
TESTS += test_float_matrix_mul
TESTS += test_float_matrix_mul_shared_mem
TESTS += test_softmax
TESTS += test_log_softmax
TESTS += test_hammer_cache
#TESTS += test_profiler
TESTS += test_tracer
TESTS += test_conv1d
TESTS += test_conv2d

regression: $(TESTS)
	@echo "CUDA REGRESSION PASSED"

$(TESTS): $(REGRESSION_PREBUILD)
	$(MAKE) -C $@ regression

clean: $(TESTS:=.clean) hardware.clean platform.clean libraries.clean link.clean

%.clean:
	$(MAKE) -C $(@:.clean=) clean

.PHONY: clean regression $(TESTS) %.clean
