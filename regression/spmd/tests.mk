# This Makefile fragment defines all of the regression tests (and the
# source path) for this sub-directory.

# Makefile.machine.include defines the Manycore hardware
# configuration.
include $(CL_DIR)/Makefile.machine.include

REGRESSION_TESTS_TYPE = spmd
SRC_PATH=$(REGRESSION_PATH)/$(REGRESSION_TESTS_TYPE)/

# "Unified tests" all use the generic test top-level:
# test_unified_main.c
UNIFIED_TESTS = test_fib 
UNIFIED_TESTS += test_bsg_print_stat
UNIFIED_TESTS += test_putchar_stream

# "Independent Tests" use a per-test <test_name>.c file
INDEPENDENT_TESTS := test_bsg_dram_loopback_cache
INDEPENDENT_TESTS += test_symbol_to_eva
INDEPENDENT_TESTS += test_bsg_loader_suite
INDEPENDENT_TESTS += test_bsg_scalar_print

# REGRESSION_TESTS is a list of all regression tests to run.
REGRESSION_TESTS = $(UNIFIED_TESTS) $(INDEPENDENT_TESTS)

DEFINES += -DBSG_MANYCORE_DIR=$(abspath $(BSG_MANYCORE_DIR))
DEFINES += -DCL_MANYCORE_DIM_X=$(CL_MANYCORE_DIM_X) 
DEFINES += -DCL_MANYCORE_DIM_Y=$(CL_MANYCORE_DIM_Y)
DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

CDEFINES   += $(DEFINES)
CXXDEFINES += $(DEFINES)

FLAGS     = -g -Wall
CFLAGS   += -std=c11 $(FLAGS) 
CXXFLAGS += -std=c++11 $(FLAGS)
