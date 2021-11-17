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
GRAPHIT_PATH := $(_REPO_ROOT)/../graphit-new
-include $(_REPO_ROOT)/environment.mk

#GRAPH_PATH := $(GRAPHIT_PATH)/test/graphs/facebook_combined.el
#FRONTIER_PATH := $(GRAPHIT_PATH)/test/graphs/fb-small-frontier.txt.2
#PARENT_PATH := $(GRAPHIT_PATH)/test/graphs/fb-small-parent.txt.2
GRAPH_PATH := $(GRAPHIT_PATH)/test/graphs/isca/hw.mtx

################################################################################
# Define BSG_MACHINE_PATH, the location of the Makefile.machine.include file
# that defines the machine to compile and simulate on. Using BSG_F1_DIR (which
# is set in environment.mk) uses the same machine as in bsg_replicant.
################################################################################

#BSG_MACHINE_PATH=$(BSG_F1_DIR)/machines/4x4_fast_n_fake
BSG_MACHINE_PATH=$(BSG_F1_DIR)/machines/timing_v0_16_8

################################################################################
# Define the range of versions
################################################################################
# Kernel versions. See kernel/README.md for more information.  Version names do
# not need to use v* and can be any string
VERSIONS = edge vertex blocked cache-aligned push cache-aligned256
BASELINEV = vertex push
OPTV = edge blocked cache-aligned
ALLV += $(OPTV)
ALLV += $(BASELINEV) 
BLOCKV = blocked16 blocked64 cache-aligned16 cache-aligned64
LBLOCKV = blocked128 cache-aligned128
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
#KERNEL_CLIBRARIES   += $(BSG_MANYCORE_DIR)/software/bsg_manycore_lib/bsg_printf.c
# C++ Libraries
KERNEL_CXXLIBRARIES +=

KERNEL_INCLUDES     += -I$(CURRENT_PATH)/kernel/include -I$(GRAPHIT_PATH)/src/runtime_lib/infra_hb/device/

# Define the default kernel.cpp file. If KERNEL_DEFAULT is not defined it will
# be set to kernel.cpp in the same directory as this Makefile.
DEFAULT_VERSION     := vertex
KERNEL_DEFAULT      := kernel/$(DEFAULT_VERSION)/kernel.cpp

################################################################################
# Include the kernel build rules (This must be included after KERNEL_*LIBRARIES,
# KERNEL_DEFAULT, KERNEL_INCLUDES, etc)
################################################################################

-include $(FRAGMENTS_PATH)/kernel/cudalite.mk

################################################################################
# END OF KERNEL-SPECIFIC RULES / START OF HOST-SPECIFIC RULES
################################################################################


################################################################################
# Define the $(HOST_TARGET), the name of the host executable to generate. The
# cosimulation host executable will be called
# $(HOST_TARGET).cosim. HOST_*SOURCES list the host files that should be
# compiled and linked into the executable.
################################################################################

HOST_TARGET         := pr
HOST_CSOURCES       := 
HOST_CXXSOURCES     := $(HOST_TARGET).cpp
HOST_INCLUDES       := -I$(CURRENT_PATH) -I$(GRAPHIT_PATH)/src/runtime_lib/

################################################################################
# Include the Cosimulation host build rules (This must be included after
# HOST_*SOURCES, HOST_TARGET, HOST_INCLUDES, etc)
################################################################################

-include $(FRAGMENTS_PATH)/host/cosim.mk

$(HOST_TARGET).log: kernel.riscv $(HOST_TARGET)
	./$(HOST_TARGET) +ntb_random_seed_automatic +rad $(SIM_ARGS) \
		+c_args="kernel.riscv $(DEFAULT_VERSION) -g $(GRAPH_PATH)" | tee $@

kernel/%/$(HOST_TARGET).log: kernel/%/kernel.riscv $(HOST_TARGET)
	$(eval EXEC_PATH   := $(patsubst %/,%,$(dir $@)))
	$(eval KERNEL_PATH := $(CURRENT_PATH)/$(EXEC_PATH))
	$(eval _VERSION    := $(notdir $(EXEC_PATH)))
	cd $(EXEC_PATH) && \
	$(CURRENT_PATH)/$(HOST_TARGET) +ntb_random_seed_automatic \ 
		+vpdfile+$(HOST_TARGET).vpd \
		+c_args="$(KERNEL_PATH)/kernel.riscv $(_VERSION) -g $(GRAPH_PATH)" | tee $(notdir $@)

################################################################################
# Define the clean rules. clean calls the makefile-specific cleans, whereas
# users can add commands and dependencies to custom.clean.
################################################################################
version.clean:
	rm -rf kernel/*/*{.csv,.log,.rvo,.riscv,.vpd,.key,.png,.dis,.ll,.ll.s}
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
default: pc_stats stats graphs

_HELP_STRING += "    analysis: \n"
_HELP_STRING += "        - Launch indpendent cosimulation executions of each kernel version.\n"
_HELP_STRING += "          When execution finishes, it generates the analysis products for \n"
_HELP_STRING += "          each kernel in each respective kernel/version_name/ directory\n"
analysis: $(foreach v,$(VERSIONS),kernel/$v/pc_stats kernel/$v/graphs kernel/$v/stats)

statistics: $(foreach v,$(VERSIONS),kernel/$v/stats)

baselines: $(foreach v,$(BASELINEV),kernel/$v/stats)
optimizations: $(foreach v,$(OPTV),kernel/$v/stats)
alltests: $(foreach v,$(ALLV),kernel/$v/stats)
blocktests: $(foreach v,$(BLOCKV),kernel/$v/stats)
largeblocks: $(foreach v,$(LBLOCKV),kernel/$v/stats)

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
