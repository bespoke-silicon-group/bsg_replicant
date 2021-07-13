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
NETSIM_PATH := $(EXAMPLES_PATH)/networksim

arg-item = $(word $(1),$(subst -, ,$(2)))

###############################################################################
# Host code compilation flags and flow
###############################################################################

# TEST_SOURCES is a list of source files that need to be compiled
TEST_SOURCES = main.cpp

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

FLAGS     = -g -Wall -Wno-unused-function -Wno-unused-variable
CFLAGS   += -std=c99 $(FLAGS)
CXXFLAGS += -std=c++14 $(FLAGS)

# compilation.mk defines rules for compilation of C/C++
include $(EXAMPLES_PATH)/compilation.mk

main.o: INCLUDES += -I$(EXAMPLES_PATH)
main.o: $(BSG_MACHINE_PATH)/bsg_manycore_machine.h

main.o bsg_dpi_tile.o: $(BSG_F1_DIR)/examples/networksim/bsg_manycore_dpi_tile.hpp
main.o bsg_dpi_tile.o: INCLUDES += -I$(BSG_F1_DIR)/examples/networksim
main.o bsg_dpi_tile.o: INCLUDES += -I.

###############################################################################
# Host code link flags and flow
###############################################################################

LDFLAGS +=

# Sources used to implement the C/C++ tile
netsim/simv: VCS_FLAGS += bsg_dpi_tile.o
netsim/simv: bsg_dpi_tile.o

# link.mk defines rules for linking of the final execution binary.
include $(NETSIM_PATH)/link.mk

# this rule does a sweep across many options
sweep: $(foreach alloc,normdist 8 9 10,$(foreach els,2 4 8 16 32 64 128,$(foreach stripe,1 2 4 8 16 32,$(foreach mode,ruchy ruchydeux ruchytrois zero xstripe ystripe,$(alloc)-$(stripe)-$(els)-$(mode).deploy))))

# this rule will generate a directory that can independently run a benchmark in parallel; and then it will run it
# syntax <reply fifo size>-<vcache stripe words>-<ruchy|ruchydeux|ruchytrois|zero|xstripe|ystripe>.deploy
# make -j 10 8-1.deploy 32-1.deploy 64-1.deploy 128-1.deploy 48-1.deploy 96-1.deploy

%.deploy:
	mkdir $@
	echo "#define CREDIT_ALLOCATION $(call arg-item,1,$*)" > $@/alloc.hpp
	echo "#define CACHE_START_TYPE \"$(call arg-item,4,$*)\"" >> $@/alloc.hpp
	cat alloc.hpp >> $@/alloc.hpp
	cp main.cpp $@/main.cpp
	cp bsg_dpi_tile.cpp $@/bsg_dpi_tile.cpp
	echo "BSG_MACHINE_PATH=machine_dir" > $@/Makefile
	cat Makefile >> $@/Makefile
	mkdir -p $@/machine_dir
	cp -r $(BSG_MACHINE_PATH)/Makefile.machine.include $@/machine_dir/
	echo "BSG_MACHINE_VCACHE_RSP_FIFO_ELS = $(call arg-item,3,$*)" >> $@/machine_dir/Makefile.machine.include
	echo "BSG_MACHINE_VCACHE_STRIPE_WORDS = $(call arg-item,2,$*)" >> $@/machine_dir/Makefile.machine.include
	$(MAKE) -C $@ go

# this will actually indirectly do the run
go: router_utilization.rpt vcache_stall_detailed.png router_utilization.png cache_utilization.png tile_execution.png

# this will summarize the data; currently just the max time for each run
# awk -F'[^0-9]*' '{print $1, $2, $3, $4}' will extract fields

summarize:
	find -iname "*.rpt" -exec grep -H Max '{}' ';'

RESULTS = $(foreach mode,ruchy ruchydeux ruchytrois zero xstripe ystripe,sum.$(mode))
$(RESULTS):
	echo "Allocation,Stripe Size (Words),Cache Response Elements,Cycles"
	find normdist* -iname "*.rpt" -exec grep -H Max '{}' ';' | grep "$(suffix $@)\." |  awk -F'[^0-9]*' '{print "normdist", $$1, $$2, $$3, $$4}' >> $@
	#find doubledist* -iname "*.rpt" -exec grep -H Max '{}' ';' | grep "$(suffix $@)\." |  awk -F'[^0-9]*' '{print "doubledist", $$1, $$2, $$3, $$4}' >> $@
	find [0-9]* -iname "*.rpt" -exec grep -H Max '{}' ';' | grep "$(suffix $@)\." |  awk -F'[^0-9]*' '{print $$1, $$2, $$3, $$4}' >> $@
	python3 $(NETSIM_PATH)/results.py $@

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
include $(NETSIM_PATH)/execution.mk

.PHONY: clean

.DEFAULT_GOAL := help
clean:
	rm -rf *.o $(TEST_NAME) *.rpt vcache_stall_detailed.png vcache_key_detailed.png *.csv

