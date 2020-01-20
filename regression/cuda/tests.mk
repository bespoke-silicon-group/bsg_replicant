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

# This Makefile fragment defines all of the regression tests (and the
# source path) for this sub-directory.

REGRESSION_TESTS_TYPE = cuda
SRC_PATH=$(REGRESSION_PATH)/$(REGRESSION_TESTS_TYPE)/

TILE_GROUP_DIM_X = 2
TILE_GROUP_DIM_Y = 2

# "Unified tests" all use the generic test top-level:
# test_unified_main.c
UNIFIED_TESTS = test_scalar_print
UNIFIED_TESTS += test_empty
UNIFIED_TESTS += test_tile_info
UNIFIED_TESTS += test_barrier

# "Independent Tests" use a per-test <test_name>.c file
INDEPENDENT_TESTS += test_binary_load_buffer
INDEPENDENT_TESTS += test_empty_parallel
INDEPENDENT_TESTS += test_multiple_binary_load
INDEPENDENT_TESTS += test_host_memset
INDEPENDENT_TESTS += test_stack_load
INDEPENDENT_TESTS += test_dram_load_store
INDEPENDENT_TESTS += test_dram_host_allocated
INDEPENDENT_TESTS += test_dram_device_allocated
INDEPENDENT_TESTS += test_device_memset
INDEPENDENT_TESTS += test_device_memcpy
INDEPENDENT_TESTS += test_vec_add
INDEPENDENT_TESTS += test_vec_add_parallel
INDEPENDENT_TESTS += test_vec_add_parallel_multi_grid
INDEPENDENT_TESTS += test_vec_add_serial_multi_grid
INDEPENDENT_TESTS += test_vec_add_shared_mem
INDEPENDENT_TESTS += test_max_pool2d
INDEPENDENT_TESTS += test_shared_mem
INDEPENDENT_TESTS += test_shared_mem_load_store
INDEPENDENT_TESTS += test_matrix_mul
INDEPENDENT_TESTS += test_matrix_mul_shared_mem

INDEPENDENT_TESTS += test_float_all_ops
INDEPENDENT_TESTS += test_float_vec_add
INDEPENDENT_TESTS += test_float_vec_add_shared_mem
INDEPENDENT_TESTS += test_float_vec_mul
INDEPENDENT_TESTS += test_float_vec_div
INDEPENDENT_TESTS += test_float_vec_exp
INDEPENDENT_TESTS += test_float_vec_sqrt
INDEPENDENT_TESTS += test_float_vec_log
INDEPENDENT_TESTS += test_float_matrix_mul
INDEPENDENT_TESTS += test_float_matrix_mul_shared_mem
INDEPENDENT_TESTS += test_softmax
INDEPENDENT_TESTS += test_log_softmax
INDEPENDENT_TESTS += test_conv1d
INDEPENDENT_TESTS += test_conv2d

# REGRESSION_TESTS is a list of all regression tests to run.
REGRESSION_TESTS = $(UNIFIED_TESTS) $(INDEPENDENT_TESTS)

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

CDEFINES   += $(DEFINES)
CXXDEFINES += $(DEFINES)

FLAGS     = -g -Wall
CFLAGS   += -std=c99 $(FLAGS) 
CXXFLAGS += -std=c++11 $(FLAGS)
