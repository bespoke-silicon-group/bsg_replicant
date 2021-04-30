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
 #  inner product with ipc=0.3 (8x4)
VERSIONS := greedy_walk
greedy_walk-grp-x := 1
greedy_walk-grp-y := 1
#  inner product with ipc=0.43 (8x4)
VERSIONS += greedy_walk_v1
greedy_walk_v1-grp-x := 1
greedy_walk_v1-grp-y := 1
#  inner product with FLOPS/cycle=0.2  (8x4)
VERSIONS += greedy_walk_v2
greedy_walk_v2-grp-x := 1
greedy_walk_v2-grp-y := 1
#  inner product with FLOPS/cycle=0.26 (8x4)
VERSIONS += greedy_walk_v3
greedy_walk_v3-grp-x := 1
greedy_walk_v3-grp-y := 1
#  inner product v4-serial
VERSIONS += greedy_walk_v3-ipv4serial
greedy_walk_v3-ipv4serial-grp-x := 1
greedy_walk_v3-ipv4serial-grp-y := 1
#  greedy_walk_v3 + ParallelInnerProduct_v1
VERSIONS += greedy_walk_v4
greedy_walk_v4-grp-x := 2
greedy_walk_v4-grp-y := 2

#  very slow - uses a very dumb sparse set
VERSIONS += beam_search
beam_search-grp-x := 1
beam_search-grp-y := 1
#  dense set - inner product with ipc=0.3  (8x4)
VERSIONS += beam_search_v1
beam_search_v1-grp-x := 1
beam_search_v1-grp-y := 1
#  dense set - inner product with ipc=0.43 (8x4)
VERSIONS += beam_search_v2
beam_search_v2-grp-x := 1
beam_search_v2-grp-y := 1
#  + inner_product_v2 (flops/cycle=0.2039) (8x4)
VERSIONS += beam_search_v3
beam_search_v3-grp-x := 1
beam_search_v3-grp-y := 1
#  + inner_product_v3 (flops/cycle=0.2663) (8x4)
VERSIONS += beam_search_v4
beam_search_v4-grp-x := 1
beam_search_v4-grp-y := 1
#  + Bit vector for dense set
VERSIONS += beam_search_v5
beam_search_v5-grp-x := 1
beam_search_v5-grp-y := 1
#  + Bit vector for dense set + inner product v4 seria;
VERSIONS += beam_search_v5-ipv4serial
beam_search_v5-ipv4serial-grp-x := 1
beam_search_v5-ipv4serial-grp-y := 1
#  beam_search_v5 + inner_product_parallel_v3
VERSIONS += beam_search_v6
beam_search_v6-grp-x := 2
beam_search_v6-grp-y := 2
# beam_search_v6 but with 1x2 tile group
VERSIONS += beam_search_v7
beam_search_v7-grp-x := 1
beam_search_v7-grp-y := 2
# beam_search_v5 but edges of candidates traversed in parallel
VERSIONS += beam_search_v8
beam_search_v8-grp-x := 4
beam_search_v8-grp-y := 4
# combination of beam_search_v8 + beam_search_v6
VERSIONS += beam_search_v9
beam_search_v9-grp-x := 4
beam_search_v9-grp-y := 4
# beam_search_v5 but edges of candidates traversed in parallel
VERSIONS += beam_search_v10
beam_search_v10-grp-x := 8
beam_search_v10-grp-y := 4

# debugging this makefile
VERSIONS += debug
debug-grp-x := 0
debug-grp-y := 0

 # baseline - ipc = 0.3
VERSIONS += iproduct_ubmk
iproduct_ubmk-grp-x := 1
iproduct_ubmk-grp-y := 1
# using clang and NBLs, ipc = 0.43, flops/cycle = 0.1867
VERSIONS += iproduct_ubmk_v1
iproduct_ubmk_v1-grp-x := 1
iproduct_ubmk_v1-grp-y := 1
# + FMA, ipc = 0.386, flops/cycle = 0.2039
VERSIONS += iproduct_ubmk_v2
iproduct_ubmk_v2-grp-x := 1
iproduct_ubmk_v2-grp-y := 1
# + explicit parallel fma (ipc=0.45,flops/cycle = 0.2663) (8x4)
VERSIONS += iproduct_ubmk_v3
iproduct_ubmk_v3-grp-x := 1
iproduct_ubmk_v3-grp-y := 1
# Slightly cleaner code than v3 - similar performance
VERSIONS += iproduct_ubmk_v4
iproduct_ubmk_v4-grp-x := 1
iproduct_ubmk_v4-grp-y := 1
 #...
VERSIONS += iproduct_ubmk_parallel
iproduct_ubmk_parallel-grp-x := 1
iproduct_ubmk_parallel-grp-y := 1
 #...
VERSIONS += iproduct_ubmk-parallel_v1
iproduct_ubmk-parallel_v1-grp-x := 2
iproduct_ubmk-parallel_v1-grp-y := 2
 #...
VERSIONS += iproduct_ubmk-parallel_v1.1
iproduct_ubmk-parallel_v1.1-grp-x := 2
iproduct_ubmk-parallel_v1.1-grp-y := 2
 #...
VERSIONS += iproduct_ubmk-parallel_v2
iproduct_ubmk-parallel_v2-grp-x := 2
iproduct_ubmk-parallel_v2-grp-y := 2
#... same as v2 but with (1x4 tg)
VERSIONS += iproduct_ubmk-parallel_v3
iproduct_ubmk-parallel_v3-grp-x := 2
iproduct_ubmk-parallel_v3-grp-y := 2

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
# Include the Cosimulation host build rules (This must be included after
# HOST_*SOURCES, HOST_TARGET, HOST_INCLUDES, etc)
################################################################################
HOST_TARGET = ipnsw
C_ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/database_music100.bin
C_ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/query_music100.bin
C_ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_0
C_ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_1
C_ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_2
C_ARGS += $(CURRENT_PATH)/hb-prog-eval/ipnsw/data/music.edges.level_3

# set group x/y values
define VERSION-SET-ARGS
kernel/$(1)/$(HOST_TARGET).log: ARGS += --group-x $($(1)-grp-x)
kernel/$(1)/$(HOST_TARGET).log: ARGS += --group-y $($(1)-grp-y)
endef
$(foreach v,$(VERSIONS),$(eval $(call VERSION-SET-ARGS,$v)))

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
kernel/iproduct_ubmk-$(1)/$(HOST_TARGET).log: C_ARGS += --num-iproducts $(1)
kernel/iproduct_ubmk-$(1)/$(HOST_TARGET).log: BSG_MANYCORE_KERNELS = kernel/iproduct_ubmk-$(1)/kernel.riscv
kernel/iproduct_ubmk-$(1)/kernel.rvo: RISCV_CXX = $(RISCV_CLANGXX)

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
iproduct-ubmk-stats: $(foreach v,$(IPRODUCT-UBMK-VERSIONS),kernel/$v/$(HOST_TARGET).exec.log)

# Add to versions
VERSIONS += $(IPRODUCT-UBMK-VERSIONS)

#########################################
# Parallel Inner Product U-Benchmarking #
#########################################
# number iproducts
N-IPRODUCTS := 1500
GRID-X := 1 2 4 8
GRID-Y := 1 2 4

IPRODUCT-PARALLEL-BASENAME := iproduct_ubmk-parallel_v1

define IPRODUCT-UBMK-PARALLEL-RULE
# creates run directory from template
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/kernel.cpp: kernel/$(IPRODUCT-PARALLEL-BASENAME)/kernel.cpp
	mkdir -p $$(dir $$@)
	cp $$< $$@

# adds arguments
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/$(HOST_TARGET).log: C_ARGS += --num-iproducts $(1)
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/$(HOST_TARGET).log: C_ARGS += --grid-x $(2)
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/$(HOST_TARGET).log: C_ARGS += --grid-y $(3)
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/$(HOST_TARGET).log: C_ARGS += --group-x $($(IPRODUCT-PARALLEL-BASENAME)-grp-x)
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/$(HOST_TARGET).log: C_ARGS += --group-y $($(IPRODUCT-PARALLEL-BASENAME)-grp-y)
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/$(HOST_TARGET).log: BSG_MANYCORE_KERNELS = kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/kernel.riscv
kernel/iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)/kernel.rvo: RISCV_CXX = $(RISCV_CLANGXX)
# adds to list of iproduct u-bmk
IPRODUCT-UBMK-PARALLEL-VERSIONS += iproduct_ubmk-parallel-$(1)-gx$(2)-gy$(3)
endef

# Expand rule for each inner product input
$(foreach gy,$(GRID-Y),$(foreach gx,$(GRID-X),$(foreach nip,$(N-IPRODUCTS), $(eval $(call IPRODUCT-UBMK-PARALLEL-RULE,$(nip),$(gx),$(gy))))))

.PHONY: create-iproduct-ubmk-parallel
.PHONY: purge-iproduct-ubmk-parallel
.PHONY: iproduct-ubmk-parallel-stats

# create rule
create-iproduct-ubmk-parallel: $(foreach v,$(IPRODUCT-UBMK-PARALLEL-VERSIONS),kernel/$v/kernel.cpp)

# purge rule
purge-iproduct-ubmk-parallel:
	rm -rf $(foreach v,$(IPRODUCT-UBMK-PARALLEL-VERSIONS),kernel/$v)

# collect stats for all
iproduct-ubmk-parallel-stats: create-iproduct-ubmk-parallel
iproduct-ubmk-parallel-stats: $(foreach v,$(IPRODUCT-UBMK-PARALLEL-VERSIONS),kernel/$v/$(HOST_TARGET).exec.log)

# Add to versions
VERSIONS += $(IPRODUCT-UBMK-PARALLEL-VERSIONS)

####################
# Greedy Walk Runs #
####################
GREEDY-WALK-QUERIES := 4 16 229 276 461 470 490
GREEDY-WALK-QUERIES := 4 16 229 276 461 470 490
GREEDY-WALK-BASENAME := greedy_walk_v4
define GREEDY-WALK-RULE
# creates run directory from template
kernel/greedy_walk-query$(1)/kernel.cpp: kernel/$(GREEDY-WALK-BASENAME)/kernel.cpp
	mkdir -p $$(dir $$@)
	cp $$< $$@

# adds arguments
kernel/greedy_walk-query$(1)/$(HOST_TARGET).exec.log: C_ARGS += --queries $(1)
kernel/greedy_walk-query$(1)/$(HOST_TARGET).exec.log: BSG_MANYCORE_KERNELS = kernel/iproduct_ubmk-$(1)/kernel.riscv
kernel/greedy_walk-query$(1)/kernel.rvo: RISCV_CXX = $(RISCV_CLANGXX)

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
greedy-walk-stats: $(foreach v,$(GREEDY-WALK-VERSIONS),kernel/$v/$(HOST_TARGET).exec.log)

# Add to versions
VERSIONS += $(GREEDY-WALK-VERSIONS)

####################
# Beam Search Runs #
####################
BEAM-SEARCH-QUERIES := 2 188 229 355 427 472
BEAM-SEARCH-QUERIES += 25  74  112 140 148 178
BEAM-SEARCH-QUERIES += 214 244 278 302 331
BEAM-SEARCH-QUERIES += 396 420 452 489 511

BEAM-SEARCH-BASENAME := beam_search_v10

define BEAM-SEARCH-RULE
# creates run directory from template
kernel/beam_search-query$(1)/kernel.cpp: kernel/$(BEAM-SEARCH-BASENAME)/kernel.cpp
	mkdir -p $$(dir $$@)
	cp $$< $$@

# adds arguments
kernel/beam_search-query$(1)/$(HOST_TARGET).log: C_ARGS += --queries $(1)
kernel/beam_search-query$(1)/$(HOST_TARGET).log: BSG_MANYCORE_KERNELS = kernel/beam_search-query$(1)/kernel.riscv
kernel/beam_search-query$$(1)/kernel.rvo: RISCV_CXX = $(RISCV_CLANGXX)

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
beam-search-stats: $(foreach v,$(BEAM-SEARCH-VERSIONS),kernel/$v/$(HOST_TARGET).exec.log)

# Add to versions
VERSIONS += $(BEAM-SEARCH-VERSIONS)

########################################
# Continue including cosim build rules #
########################################

GRAPH-TOOLS := $(EXAMPLES_PATH)/sdh-eval-workloads/ipnsw/graph-tools
graphtools-dir := $(GRAPH-TOOLS)

include $(GRAPH-TOOLS)/libgraphtools.mk

HB-HELPERS := $(EXAMPLES_PATH)/sdh-eval-workloads/ipnsw/hammerblade-helpers
hammerblade-helpers-dir := $(HB-HELPERS)
include $(HB-HELPERS)/libhammerblade-helpers-host.mk

CXXFLAGS += $(libhammerblade-helpers-host-interface-cxxflags)
CXXFLAGS += $(libgraphtools-interface-cxxflags)
CXXFLAGS += -I$(EXAMPLES_PATH)/sdh-eval-workloads/ipnsw
CXXFLAGS += -DCOSIM

LDFLAGS  += $(libhammerblade-helpers-host-interface-ldflags)
LDFLAGS  += $(libgraphtools-interface-ldflags)

GreedyWalkResults.o: $(libhammerblade-helpers-host-interface-headers)
GreedyWalkResults.o: $(libgraphtools-interface-headers)
GreedyWalkResults.o: $(libgraphtools-interface-libraries)
GreedyWalkResults.o: GreedyWalkResults.cpp
GreedyWalkResults.o: GreedyWalkResults.hpp

ipnsw.o: $(libhammerblade-helpers-host-interface-headers)
ipnsw.o: $(libgraphtools-interface-headers)
ipnsw.o: $(libgraphtools-interface-libraries)
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

TEST_SOURCES = ipnsw.cpp GreedyWalkResults.cpp

-include $(EXAMPLES_PATH)/compilation.mk
-include $(EXAMPLES_PATH)/link.mk
-include $(EXAMPLES_PATH)/execution.mk

################################################################################
# Define the clean rules. clean calls the makefile-specific cleans, whereas
# users can add commands and dependencies to custom.clean.
################################################################################
version.clean:
	rm -rf kernel/*/*{.csv,.log,.rvo,.riscv,.vpd,.key,.png,.dis}
	rm -rf kernel/*/{stats,pc_stats}

custom.clean: version.clean

