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

################################################################################
# Paths / Environment Configuration
################################################################################
_REPO_ROOT ?= $(shell git rev-parse --show-toplevel)
CURRENT_PATH := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

-include $(_REPO_ROOT)/environment.mk

################################################################################
# Define BSG_MACHINE_PATH, the location of the Makefile.machine.include file
# that defines the machine to compile and simulate on. Using BSG_F1_DIR (which
# is set in environment.mk) uses the same machine as in bsg_replicant.
################################################################################

BSG_MACHINE_PATH=$(BSG_F1_DIR)/machines/pod_X1Y1_ruche_X16Y8_hbm

################################################################################
# Define the range of versions
################################################################################
# Kernel versions. See kernel/README.md for more information.  Version names do
# not need to use v* and can be any string
VERSIONS := greedy_walk    #  inner product with ipc=0.3 (8x4)
VERSIONS += greedy_walk_v1 #  inner product with ipc=0.43 (8x4)
VERSIONS += greedy_walk_v2 #  inner product with FLOPS/cycle=0.2  (8x4)
VERSIONS += greedy_walk_v3 #  inner product with FLOPS/cycle=0.26 (8x4)
VERSIONS += beam_search    #  very slow - uses a very dumb sparse set
VERSIONS += beam_search_v1 #  dense set - inner product with ipc=0.3  (8x4)
VERSIONS += beam_search_v2 #  dense set - inner product with ipc=0.43 (8x4)
VERSIONS += beam_search_v3 #  + inner_product_v2 (flops/cycle=0.2039) (8x4)
VERSIONS += beam_search_v4 #  + inner_product_v3 (flops/cycle=0.2663) (8x4)
VERSIONS += beam_search_v5 #  + Bit vector for dense set
VERSIONS += debug
VERSIONS += iproduct_ubmk # baseline - ipc = 0.3
VERSIONS += iproduct_ubmk_v1 # using clang and NBLs, ipc = 0.43, flops/cycle = 0.1867
VERSIONS += iproduct_ubmk_v2 # + FMA, ipc = 0.386, flops/cycle = 0.2039
VERSIONS += iproduct_ubmk_v3 # + explicit parallel fma (ipc=0.45,flops/cycle = 0.2663) (8x4)
VERSIONS += iproduct_ubmk_v4 # Slightly cleaner code than v3 - similar performance

_KERNEL_COMPILER = CLANG
################################################################################
# Define any sources that should be used compiled during kernel compilation,
# including the source file with the kernel itself. kernel.riscv will
# be the name of the compiled RISC-V Binary for the Manycore
#
# Use KERNEL_*LIBRARIES list sources that should be compiled and linked with all
# kernel.cpp versions. However, if you have version-specific sources you must
# come up with your own solution.
# 
# Use KERNEL_INCLUDES to specify the path to directories that contain headers.
################################################################################

# C Libraries
KERNEL_CLIBRARIES   +=
# C++ Libraries
KERNEL_CXXLIBRARIES +=

KERNEL_INCLUDES     += -I$(CURRENT_PATH)/kernel/include

# Define the default kernel.cpp file. If KERNEL_DEFAULT is not defined it will
# be set to kernel.cpp in the same directory as this Makefile.
DEFAULT_VERSION     := greedy_walk_v3
KERNEL_DEFAULT      := kernel/$(DEFAULT_VERSION)/kernel.cpp
#KERNEL_DEFAULT      := kernel/$(DEFAULT_VERSION)/kernel.c

################################################################################
# Include the kernel build rules (This must be included after KERNEL_*LIBRARIES,
# KERNEL_DEFAULT, KERNEL_INCLUDES, etc)
################################################################################

-include $(EXAMPLES_PATH)/examples/cuda/riscv.mk

################################################################################
# END OF KERNEL-SPECIFIC RULES / START OF HOST-SPECIFIC RULES
################################################################################


################################################################################
# Define the $(HOST_TARGET), the name of the host executable to generate. The
# cosimulation host executable will be called
# $(HOST_TARGET).cosim. HOST_*SOURCES list the host files that should be
# compiled and linked into the executable.
################################################################################

HOST_TARGET         := ipnsw
HOST_CSOURCES       := 
HOST_CXXSOURCES     += GreedyWalkResults.cpp
HOST_INCLUDES       := -I$(CURRENT_PATH)

################################################################################
# Include the Cosimulation host build rules (This must be included after
# HOST_*SOURCES, HOST_TARGET, HOST_INCLUDES, etc)
################################################################################

ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/database_music100.bin
ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/query_music100.bin
ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_0
ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_1
ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_2
ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_3


################################
# Inner Product U-Benchmarking #
################################
# number iproducts
N-IPRODUCTS := 150 500 1000 1500 2000 3000
IPRODUCT-BASENAME := iproduct_ubmk_v4

define IPRODUCT-UBMK-RULE
# creates run directory from template
kernel/iproduct_ubmk-$(1)/kernel.cpp: kernel/$(IPRODUCT-BASENAME)/kernel.cpp
	mkdir -p $$(dir $$@)
	cp $$< $$@

# adds arguments
kernel/iproduct_ubmk-$(1)/$(HOST_TARGET).log: ARGS += --num-iproducts $(1)

# adds to list of iproduct u-bmk
IPRODUCT-UBMK-VERSIONS += iproduct_ubmk-$(1)
endef

# Expand rule for each inner product input
$(foreach nip,$(N-IPRODUCTS),$(eval $(call IPRODUCT-UBMK-RULE,$(nip))))

.PHONY: create-iproduct-ubmk
.PHONY: purge-iproduct-ubmk
.PHONY: iproduct-ubmk-stats

# create rule
create-iproduct-ubmk: $(foreach v,$(IPRODUCT-UBMK-VERSIONS),kernel/$v/kernel.cpp)

# purge rule
purge-iproduct-ubmk:
	rm -rf $(foreach v,$(IPRODUCT-UBMK-VERSIONS),kernel/$v)

# collect stats for all
iproduct-ubmk-stats: create-iproduct-ubmk
iproduct-ubmk-stats: $(foreach v,$(IPRODUCT-UBMK-VERSIONS),kernel/$v/stats)

# Add to versions
VERSIONS += $(IPRODUCT-UBMK-VERSIONS)

####################
# Greedy Walk Runs #
####################
GREEDY-WALK-QUERIES := 4 16 229 276 461 470 490
GREEDY-WALK-BASENAME := greedy_walk_v3
define GREEDY-WALK-RULE
# creates run directory from template
kernel/greedy_walk-query$(1)/kernel.cpp: kernel/$(GREEDY-WALK-BASENAME)/kernel.cpp
	mkdir -p $$(dir $$@)
	cp $$< $$@

# adds arguments
kernel/greedy_walk-query$(1)/$(HOST_TARGET).log: ARGS += --queries $(1)

# adds to list of greedy walk versions
GREEDY-WALK-VERSIONS += greedy_walk-query$(1)
endef

# Expand rule for each query
$(foreach q,$(GREEDY-WALK-QUERIES),$(eval $(call GREEDY-WALK-RULE,$(q))))

.PHONY: create-greedy-walk
.PHONY: purge-greedy-walk
.PHONY: greedy-walk-stats

# create rule
create-greedy-walk: $(foreach v,$(GREEDY-WALK-VERSIONS),kernel/$v/kernel.cpp)

# purge rule
purge-greedy-walk:
	rm -rf $(foreach v,$(GREEDY-WALK-VERSIONS),kernel/$v)

# collect stats for all
greedy-walk-stats: create-greedy-walk
greedy-walk-stats: $(foreach v,$(GREEDY-WALK-VERSIONS),kernel/$v/stats)

# Add to versions
VERSIONS += $(GREEDY-WALK-VERSIONS)

####################
# Beam Search Runs #
####################
BEAM-SEARCH-QUERIES := 2 188 229 355 427 472
BEAM-SEARCH-BASENAME := beam_search_v5

define BEAM-SEARCH-RULE
# creates run directory from template
kernel/beam_search-query$(1)/kernel.cpp: kernel/$(BEAM-SEARCH-BASENAME)/kernel.cpp
	mkdir -p $$(dir $$@)
	cp $$< $$@

# adds arguments
kernel/beam_search-query$(1)/$(HOST_TARGET).log: ARGS += --queries $(1)

# adds to list of greedy walk versions
BEAM-SEARCH-VERSIONS += beam_search-query$(1)
endef


# Expand rule for each query
$(foreach q,$(BEAM-SEARCH-QUERIES),$(eval $(call BEAM-SEARCH-RULE,$(q))))

.PHONY: create-beam-search
.PHONY: purge-beam-search
.PHONY: beam-search-stats

# create rule
create-beam-search: $(foreach v,$(BEAM-SEARCH-VERSIONS),kernel/$v/kernel.cpp)

# purge rule
purge-beam-search:
	rm -rf $(foreach v,$(BEAM-SEARCH-VERSIONS),kernel/$v)

# collect stats for all
beam-search-stats: create-beam-search
beam-search-stats: $(foreach v,$(BEAM-SEARCH-VERSIONS),kernel/$v/stats)

# Add to versions
VERSIONS += $(BEAM-SEARCH-VERSIONS)

########################################
# Continue including cosim build rules #
########################################

-include $(FRAGMENTS_PATH)/host/cosim.mk

GRAPH-TOOLS := $(CURRENT_PATH)/graph-tools
graphtools-dir := $(GRAPH-TOOLS)

include $(GRAPH-TOOLS)/libgraphtools.mk

HB-HELPERS := $(CURRENT_PATH)/hammerblade-helpers
include $(HB-HELPERS)/libhammerblade-helpers-host.mk

CXXFLAGS += $(libhammerblade-helpers-host-interface-cxxflags)
CXXFLAGS += $(libgraphtools-interface-cxxflags)

LDFLAGS  += $(libhammerblade-helpers-host-interface-ldflags)
LDFLAGS  += $(libgraphtools-interface-ldflags)
VSOURCES += GreedyWalkResults.cpp

$(HOST_TARGET): $(libhammerblade-helpers-host-interface-headers)
$(HOST_TARGET): $(libgraphtools-interface-headers)
$(HOST_TARGET): $(libgraphtools-interface-libraries)
$(HOST_TARGET): GreedyWalkResults.o

GreedyWalkResults.o: GreedyWalkResults.cpp
GreedyWalkResults.o: GreedyWalkResults.hpp

ipnsw.o: IO.hpp
ipnsw.o: IPNSWGraph.hpp
ipnsw.o: IPNSWRunner.hpp
ipnsw.o: IPNSWKernelRunner.hpp
ipnsw.o: GreedyWalkKernelRunner.hpp
ipnsw.o: BeamSearchKernelRunner.hpp
ipnsw.o: IProductUBmkKernelRunner.hpp
ipnsw.o: IPNSWResultReader.hpp
ipnsw.o: GreedyWalkResultReader.hpp
ipnsw.o: BeamSearchResultReader.hpp
ipnsw.o: GreedyWalkResults.hpp
ipnsw.o: IPNSWFactory.hpp
ipnsw.o: GreedyWalkFactory.hpp
ipnsw.o: BeamSearchFactory.hpp
ipnsw.o: IProductUBmkFactory.hpp
ipnsw.o: StringHelpers.hpp
################################################################################
# Define the clean rules. clean calls the makefile-specific cleans, whereas
# users can add commands and dependencies to custom.clean.
################################################################################
version.clean:
	rm -rf kernel/*/*{.csv,.log,.rvo,.riscv,.vpd,.key,.png,.dis}
	rm -rf kernel/*/{stats,pc_stats}

custom.clean: version.clean

clean: cosim.clean analysis.clean cudalite.clean custom.clean

################################################################################
# Define overall-goals. The all rule runs all kernel versions, and the default
# kernel.
################################################################################

_HELP_STRING := "Makefile Rules\n"

_HELP_STRING += "    default: \n"
_HELP_STRING += "        - Run the default kernel ($KERNEL_DEFAULT) and generate all of the\n"
_HELP_STRING += "          analysis products\n"
default: pc_stats graphs stats

_HELP_STRING += "    analysis: \n"
_HELP_STRING += "        - Launch indpendent cosimulation executions of each kernel version.\n"
_HELP_STRING += "          When execution finishes, it generates all the analysis products \n"
_HELP_STRING += "          for each kernel in each respective kernel/<version_name>/ \n"
_HELP_STRING += "          directory\n"
analysis: $(foreach v,$(VERSIONS),kernel/$v/pc_stats kernel/$v/graphs kernel/$v/stats)

_HELP_STRING += "    statistics: \n"
_HELP_STRING += "        - Launch indpendent cosimulation executions of each kernel version.\n"
_HELP_STRING += "          When execution finishes, it generates ONLY the parsed operation \n"
_HELP_STRING += "          stats for each kernel in each respective kernel/<version_name>/ \n"
_HELP_STRING += "          directory\n"
statistics: $(foreach v,$(VERSIONS),kernel/$v/stats)

_HELP_STRING += "    all: \n"
_HELP_STRING += "        - Launch both the default and analysis target\n"
all: analysis default

.DEFAULT_GOAL = help
_HELP_STRING += "    help: \n"
_HELP_STRING += "        - Output a friendly help message.\n"
help:
	@echo -e $(HELP_STRING)

# Always re-run, if asked.
.PHONY: default analysis help

# These last three lines ensure that _HELP_STRING is appended to the top of
# whatever else comes before it.
_HELP_STRING += "\n"
_HELP_STRING += $(HELP_STRING)
HELP_STRING := $(_HELP_STRING)

