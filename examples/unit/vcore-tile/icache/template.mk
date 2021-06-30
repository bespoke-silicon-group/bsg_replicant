REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk

# TEST_NAME is the basename of the executable
TEST_NAME = main

bsg_group_size = 0
include $(BSG_MANYCORE_DIR)/software/mk/Makefile.paths
include $(BSG_MANYCORE_DIR)/software/mk/Makefile.builddefs
RISCV_LINK_OPTS_FILTER_OUT := -l:crt.o
RISCV_LINK_OPTS := $(filter-out,$(RISCV_LINK_OPTS_FILTER_OUT),$(RISCV_LINK_OPTS))
RISCV_LINK_OPTS += -nostartfiles -nostdlib
device.riscv: $(LINK_SCRIPT) device.o
	$(RISCV_LINK) device.o -o $@ $(RISCV_LINK_OPTS)

###############################################################################
# Host code compilation flags and flow
###############################################################################

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = main.cpp

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CDEFINES += 
CXXDEFINES += 

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++11 $(FLAGS)

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

###############################################################################
# Host code link flags and flow
###############################################################################

LDFLAGS +=

# link.mk defines rules for linking of the final execution binary.
include $(EXAMPLES_PATH)/link.mk
$(TEST_OBJECTS): device.riscv

vpath %.cpp $(EXAMPLES_PATH)/unit/vcore-tile/icache
vpath %.c   $(EXAMPLES_PATH)/unit/vcore-tile/icache
vpath %.S   $(EXAMPLES_PATH)/unit/vcore-tile/icache

###############################################################################
# Execution flow
#
# C_ARGS: Use this to pass arguments that you want to appear in argv
#
# SIM_ARGS: Use this to pass arguments to the simulator
###############################################################################
C_ARGS ?=

SIM_ARGS ?=

# Include platform-specific execution rules
include $(EXAMPLES_PATH)/execution.mk

###############################################################################
# Regression Flow
###############################################################################

regression: exec.log
	@grep "BSG REGRESSION TEST .*PASSED.*" $< > /dev/null

###############################################################################
# Default rules, help, and clean
###############################################################################
.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {clean | $(TEST_NAME).{profile,debug} | $(TEST_NAME).{profile,debug}.log}"
	@echo "      $(TEST_NAME).profile: Build executable with profilers enabled"
	@echo "      $(TEST_NAME).debug: Build waveform executable (if VCS)"
	@echo "      $(TEST_NAME).{profile,debug}.log: Run specific executable"
	@echo "      clean: Remove all subdirectory-specific outputs"


.PHONY: clean

clean:
