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

# This Makefile is for running the examples in bsg_replicant
#

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# LIBRAIRES_PATH: The path to the libraries directory
# HARDWARE_PATH: The path to the hardware directory
# EXAMPLES_PATH: The path to the examples directory
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../environment.mk

include $(EXAMPLES_PATH)/link.mk

# Supported example suites
TARGETS = library spmd cuda python

regression: $(BSG_MACHINE_PATH)/regression.log
$(BSG_MACHINE_PATH)/regression.log: regression.log
	cp $< $@

regression.log: $(TARGETS)
	@cat $(foreach tgt,$(TARGETS),$(tgt)/regression.log) | tee $@

# This is kinda weird, but it guarantees that the platform libraries
# are built before recursing. We call it PHONY so that it always gets
# re-built
.PHONY: library/test_rom
library/test_rom:
	$(MAKE) -C library test_rom

$(TARGETS): library/test_rom
	$(MAKE) -C $@ regression

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<subdirectory_name>}"
	@echo "      regression: Run all tests in all subdirectories"
	@echo "      <subdirectory_name>: Run all the regression tests for"
	@echo "             a specific sub-directory (Options are: $(TARGETS))"
	@echo "      clean: Remove all build files"

clean: hardware.clean libraries.clean
	$(foreach t,$(TARGETS), $(MAKE) -C $t clean;)
	rm -rf regression.log runtime.log

.PHONY: help clean regression $(TARGETS)
