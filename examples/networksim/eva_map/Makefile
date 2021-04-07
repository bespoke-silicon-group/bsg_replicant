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

REPLICANT-TOP:=$(shell git rev-parse --show-toplevel)
PY_ANALYTICS_PATH := $(REPLICANT-TOP)/examples/networksim

include $(REPLICANT-TOP)/environment.mk
$(warning $(PY_ANALYTICS_PATH))

TEST_NAME = main

arg-item = $(word $(1),$(subst -, ,$(2)))

###############################################################################
# Host code compilation flags and flow
###############################################################################

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = $(TEST_NAME).cpp

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++14 $(FLAGS)

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

# Again, TEST_NAME is not required
$(TEST_NAME).o: INCLUDES += -I$(EXAMPLES_PATH)
$(TEST_NAME).o: INCLUDES += -I$(BSG_F1_DIR)/examples/networksim
$(TEST_NAME).o: $(BSG_MACHINE_PATH)/bsg_manycore_machine.h
$(TEST_NAME).o: $(BSG_F1_DIR)/examples/networksim/bsg_manycore_dpi_tile.hpp

$(TEST_NAME).log: $(TEST_NAME)

###############################################################################
# Host code link flags and flow
###############################################################################

LDFLAGS +=

# link.mk defines rules for linking of the final execution binary.
include $(EXAMPLES_PATH)/link.mk

router_utilization.rpt: $(TEST_NAME).netsim.log
	python3 $(PY_ANALYTICS_PATH)/router_parser.py

tile_execution.rpt: $(TEST_NAME).netsim.log
	python3 $(PY_ANALYTICS_PATH)/tileparser.py

%.png: %.rpt
	pango-view  --font mono -qo $@ $<

cache_utilization.png: $(TEST_NAME).netsim.log
	python3 $(PY_ANALYTICS_PATH)/cacheutil.py

.PHONY: router_utilization.rpt vcache_stall_detailed.png

vcache_stall_detailed.png: $(TEST_NAME).netsim.log
	PYTHONPATH=$(BSG_MANYCORE_DIR)/software/py/vanilla_parser/.. python3 -m vanilla_parser --only vcache_stall_graph --trace vcache_operation_trace.csv --generate-key

# this rule does a sweep across many options
sweep: $(foreach alloc,normdist 8 9 10,$(foreach els,2 4 8 16 32,$(foreach stripe,1 2 4 8 16 32,$(foreach mode,ruchy ruchydeux ruchytrois zero xstripe ystripe,$(alloc)-$(stripe)-$(els)-$(mode).deploy))))

# this rule will generate a directory that can independently run a benchmark in parallel; and then it will run it
# syntax <reply fifo size>-<vcache stripe words>-<ruchy|ruchydeux|ruchytrois|zero|xstripe|ystripe>.deploy
# make -j 10 8-1.deploy 32-1.deploy 64-1.deploy 128-1.deploy 48-1.deploy 96-1.deploy

%.deploy:
	mkdir $@
	echo "#define CREDIT_ALLOCATION $(call arg-item,1,$*)" > $@/$(TEST_NAME).cpp
	echo "#define CACHE_START_TYPE \"$(call arg-item,4,$*)\"" >> $@/$(TEST_NAME).cpp
	cat $(TEST_NAME).cpp >> $@/$(TEST_NAME).cpp
	echo "BSG_MACHINE_PATH=my_machine" > $@/Makefile
	cat Makefile >> $@/Makefile
	mkdir -p $@/my_machine
	cp -r $(BSG_MACHINE_PATH)/Makefile.machine.include $@/my_machine/.
	echo "BSG_MACHINE_VCACHE_RSP_FIFO_ELS = $(call arg-item,3,$*)" >> $@/my_machine/Makefile.machine.include
	echo "BSG_MACHINE_VCACHE_STRIPE_WORDS = $(call arg-item,2,$*)" >> $@/my_machine/Makefile.machine.include
	$(MAKE) -C $@ go

# this will actually indirectly do the run
go: router_utilization.rpt vcache_stall_detailed.png router_utilization.png cache_utilization.png tile_execution.png

# this will summarize the data; currently just the max time for each run
# awk -F'[^0-9]*' '{print $1, $2, $3, $4}' will extract fields

summarize:
	find -iname "*.rpt" -exec grep -H Max '{}' ';'

RESULTS = $(foreach mode,ruchy ruchydeux ruchytrois zero xstripe ystripe,sum.$(mode))
$(RESULTS):
	find normdist* -iname "*.rpt" -exec grep -H Max '{}' ';' | grep "$(suffix $@)\." |  awk -F'[^0-9]*' '{print "normdist", $$1, $$2, $$3, $$4}' >> $@
	#find doubledist* -iname "*.rpt" -exec grep -H Max '{}' ';' | grep "$(suffix $@)\." |  awk -F'[^0-9]*' '{print "doubledist", $$1, $$2, $$3, $$4}' >> $@
	find [0-9]* -iname "*.rpt" -exec grep -H Max '{}' ';' | grep "$(suffix $@)\." |  awk -F'[^0-9]*' '{print $$1, $$2, $$3, $$4}' >> $@
	python3 $(PY_ANALYTICS_PATH)/results.py $@

summarize-by-hash: $(RESULTS)

###############################################################################
# C_ARGS: Use this to pass arguments that you want to appear in argc/argv
#
# SIM_ARGS: Use this to pass arguments to the simulator
#
###############################################################################
C_ARGS ?=

SIM_ARGS ?=

# Include platform-specific execution rules
include $(EXAMPLES_PATH)/execution.mk

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {clean|<test_name>|<test_name>.log}"
	@echo "      $(TEST_NAME): Build executable for this platform"
	@echo "      $(TEST_NAME).log: Run test and generate the log file"
	@echo "      $(TEST_NAME).debug: Build waveform executable (if VCS)"
	@echo "      clean: Remove all subdirectory-specific outputs"


.PHONY: clean

clean:
	rm -rf *.o $(TEST_NAME) *.rpt vcache_stall_detailed.png vcache_key_detailed.png *.csv

