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

# This Makefile is for running cosimulation regression
#
# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../environment.mk

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly.
#
# *environment.mk must be included before cadenv.mk*
include $(CL_DIR)/cadenv.mk

# simlibs.mk defines build rules for hardware and software simulation libraries
# that are necessary for running cosimulation. These are dependencies for
# regression since running $(MAKE) recursively does not prevent parallel builds
# of identical rules -- which causes errors.
#
include simlibs.mk


# targets.mk defines the targets (sub-directories) for regression in
# cosimulation
include $(REGRESSION_PATH)/targets.mk
.PHONY: clean %.clean regression $(TARGETS)

regression: $(BSG_MACHINE_PATH)/regression.log $(BSG_MACHINE_PATH)/runtime.log

$(BSG_MACHINE_PATH)/regression.log: regression.log
	cp $< $@

regression.log: $(TARGETS)
	@cat $(foreach tgt,$(TARGETS),$(tgt)/regression.log) | tee $@

runtime.log: $(BSG_MACHINE_PATH)/regression.log
	@grep -H "CPU Time:" */test_*.log > $@
	@sed -i 's/:CPU Time:\s*\([0-9]*.[0-9]*\).*/,\1/' $@
	@sed -i 's/.log//'  $@
	@sed -i 's/\//,/'  $@
	@sed -i "s/^\(.*\)/$(CL_MANYCORE_DIM_X),$(CL_MANYCORE_DIM_Y),$(CL_MANYCORE_MEM_CFG),\1/" $@

$(BSG_MACHINE_PATH)/runtime.log: runtime.log
	cp $< $@

$(TARGETS): $(SIMLIBS)
	$(MAKE) -C $@ regression

clean: $(addsuffix .clean,$(TARGETS)) simlibs.clean
	rm -rf *.log *.jou 
	rm -rf *.vanilla_operation_trace.csv *.vanilla_stats.csv
	rm -rf .cxl*
	rm -rf *.bak

%.clean:
	$(MAKE) -C $(basename $@) clean
	rm -rf .Xil

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<subdirectory_name>}"
	@echo "      regression: Run all cosimulation regression tests"
	@echo "      <subdirectory_name>: Run all the regression tests for"
	@echo "             a specific sub-directory (Options are: $(TARGETS))"
	@echo "      clean: Remove all cosimulation build files"
