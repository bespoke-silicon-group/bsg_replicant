# Copyright (c) 2021, University of Washington All rights reserved.
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


REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

repos := hammerblade-helpers graph-tools

hammerblade-helpers.url    = git@github.com:mrutt92/hammerblade-helpers
hammerblade-helpers.commit = master

graph-tools.url    = git@github.com:mrutt92/graph-tools
graph-tools.commit = master

$(repos): %:
	git clone $($*.url)
	cd $* && git checkout $($*.commit)
	cd $* && git submodule update --init

checkout_repos: $(repos)

include $(REPLICANT_PATH)/environment.mk

include $(EXAMPLES_PATH)/cuda/bfs/config.mk

# $1 = graph_type
# $2 = vertices
# $3 = edges

graph-name = $(1)_graph-type__$(2)_vertices__$(3)_edges

# $1 = graph-name
type-from-graph-name	 = $(firstword $(subst _, ,$(filter %_graph-type,$(subst __, ,$1))))
vertices-from-graph-name = $(firstword $(subst _, ,$(filter %_vertices,$(subst __, ,$1))))
edges-from-graph-name	 = $(firstword $(subst _, ,$(filter %_edges,$(subst __, ,$1))))

# $1 = graph-name
define set-graph-parameters
$(eval GRAPH_TYPE=$(call type-from-graph-name,$1))
$(eval VERTICES=$(call vertices-from-graph-name,$1))
$(eval EDGES=$(call edges-from-graph-name,$1))
endef

GRAPHS += $(call graph-name,$(TEST_GRAPH_TYPE),$(TEST_VERTICES),$(TEST_EDGES))
GRAPHS := $(addprefix inputs/,$(GRAPHS))
GRAPHS_ANALYSIS := $(addsuffix .analysis.txt,$(GRAPHS))

$(GRAPHS): inputs/%: input-gen
	$(call set-graph-parameters,$*)
	@mkdir -p $(dir $@)
	./$< $(GRAPH_TYPE) $(VERTICES) $(EDGES) $@

$(GRAPHS_ANALYSIS): inputs/%.analysis.txt: inputs/% input-analyze
	./input-analyze $< avg 10 $@

.PHONY: inputs
inputs: $(GRAPHS)
inputs-analysis: $(GRAPHS_ANALYSIS)

# $1 = graph-type
# $2 = graph-vertices
# $3 = graph-edges
# $4 = bfs-root
# $5 = bfs-iteration
# $6 = tile-groups
# $7 = tgx
# $8 = tgy

test-name = $(1)_graph-type__$(2)_vertices__$(3)_edges__$(4)_root__$(5)_iter__$(6)_tile-groups__$(7)_tgx__$(8)_tgy

# $1 = test name
get-graph-type	= $(firstword $(subst _, ,$(filter %_graph-type,$(subst __, ,$1))))
get-vertices	= $(firstword $(subst _, ,$(filter %_vertices,$(subst __, ,$1))))
get-edges	= $(firstword $(subst _, ,$(filter %_edges,$(subst __, ,$1))))
get-root	= $(firstword $(subst _, ,$(filter %_root,$(subst __, ,$1))))
get-iter	= $(firstword $(subst _, ,$(filter %_iter,$(subst __, ,$1))))
get-tile-groups = $(firstword $(subst _, ,$(filter %_tile-groups,$(subst __, ,$1))))
get-tgx		= $(firstword $(subst _, ,$(filter %_tgx,$(subst __, ,$1))))
get-tgy		= $(firstword $(subst _, ,$(filter %_tgy,$(subst __, ,$1))))

define get-test-parameters
$(eval GRAPH_TYPE=$(call get-graph-type,$1))
$(eval VERTICES=$(call get-vertices,$1))
$(eval EDGES=$(call get-edges,$1))
$(eval ROOT=$(call get-root,$1))
$(eval ITER=$(call get-iter,$1))
$(eval TILE_GROUPS=$(call get-tile-groups,$1))
$(eval TGX=$(call get-tgx,$1))
$(eval TGY=$(call get-tgy,$1))
endef

TEST_THREADS = 32 64 128
$(foreach threads,$(TEST_THREADS),\
$(foreach iter,$(TEST_ITERS),\
$(eval TESTS += $(call test-name,$(TEST_GRAPH_TYPE),$(TEST_VERTICES),$(TEST_EDGES),$(TEST_ROOT_NODE),$(iter),$(threads),1,1))))

# simulation directory generation
TESTS_MAKEFILE  = $(addsuffix /Makefile,$(TESTS))
TESTS_PARAMETERS_MK = $(addsuffix /parameters.mk,$(TESTS))

# generate a parameter makefile
$(TESTS_PARAMETERS_MK): %/parameters.mk:
	$(call get-test-parameters,$*)
	@echo "Creating $@"
	@mkdir -p $*
	@echo RISCV_PATH = $(EXAMPLES_PATH)/cuda/bfs/$*/main.riscv > $@
	@echo GRAPH_TYPE = $(GRAPH_TYPE) >> $@
	@echo GRAPH_VERTICES = $(VERTICES) >> $@
	@echo GRAPH_EDGES = $(EDGES) >> $@
	@echo BFS_ROOT = $(ROOT) >> $@
	@echo BFS_ITERATION = $(ITER) >> $@
	@echo TILE_GROUPS = $(TILE_GROUPS) >> $@
	@echo TILE_GROUP_DIM_X = $(TGX) >> $@
	@echo TILE_GROUP_DIM_Y = $(TGY) >> $@
	@echo INPUT_GRAPH_PATH = $(EXAMPLES_PATH)/cuda/bfs/inputs/$(call graph-name,$(GRAPH_TYPE),$(VERTICES),$(EDGES)) >> $@

# copy the template to the the target directory
$(TESTS_MAKEFILE): %/Makefile: template.mk
	@echo "Creating $@"
	@mkdir -p $*
	@cp $< $@

.PHONY: tests_makefile tests_parameters_mk tests_dir
tests_makefile: $(TESTS_MAKEFILE)
tests_parameters_mk: $(TESTS_PARAMETERS_MK)
tests_dir: tests_makefile tests_parameters_mk

$(TESTS): checkout_repos
$(TESTS): %: %/Makefile %/parameters.mk

TESTS_EXEC	= $(addsuffix .exec,$(TESTS))
TESTS_PROFILE	= $(addsuffix .profile,$(TESTS))
TESTS_DEBUG	= $(addsuffix .debug,$(TESTS))
TESTS_SAIFGEN	= $(addsuffix .saifgen,$(TESTS))

$(TESTS_EXEC): %.exec: %
$(TESTS_PROFILE): %.profile: %
$(TESTS_DEBUG): %.debug: %
$(TESTS_SAIFGEN): %.saifgen: %
$(TESTS_EXEC) $(TESTS_PROFILE) $(TESTS_DEBUG) $(TESTS_SAIFGEN): $(GRAPHS)
$(TESTS_EXEC) $(TESTS_PROFILE) $(TESTS_DEBUG) $(TESTS_SAIFGEN): $(GRAPHS_ANALYSIS)
$(TESTS_EXEC) $(TESTS_PROFILE) $(TESTS_DEBUG) $(TESTS_SAIFGEN):
	$(eval RUN_DIR=$(firstword $(subst ., ,$@)))
	$(eval RUN_TYPE=$(lastword $(subst ., ,$@)))
	$(MAKE) -C $(RUN_DIR) $(RUN_TYPE).log

exec: $(TESTS_EXEC)
profile: $(TESTS_PROFILE)
debug: $(TESTS_DEBUG)
saifgen: $(TESTS_SAIFGEN)

TESTS_BLOOD = $(addsuffix .blood,$(TESTS))
$(TESTS_BLOOD): %.blood: %
	$(MAKE) -C $* blood

blood: $(TESTS_BLOOD)

# some support executables
graphtools-dir = $(EXAMPLES_PATH)/cuda/bfs/graph-tools
-include $(graphtools-dir)/libgraphtools.mk

SUPPORT_TOOLS = input-gen input-analyze input-degree-list
$(SUPPORT_TOOLS): checkout_repos
$(SUPPORT_TOOLS): $(libgraphtools-interface-headers)
$(SUPPORT_TOOLS): $(libgraphtools-interface-libraries)
$(SUPPORT_TOOLS): CXXFLAGS+=$(libgraphtools-interface-cxxflags)
$(SUPPORT_TOOLS): CXXFLAGS+=$(libgraphtools-interface-ldflags)
$(SUPPORT_TOOLS): %: %.cpp
$(SUPPORT_TOOLS):
	$(CXX) $(filter %.cpp,$^) $(CXXFLAGS) -o $@

support-tools: $(SUPPORT_TOOLS)

dramsim3.summary.csv: $(addsuffix /dramsim3.tag.json,$(TESTS))
	@echo Creating $@
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/bfs/hammerblade-helpers/py python3 $(EXAMPLES_PATH)/cuda/bfs/py/dramsim3.py $^

vcache.summary.csv: $(addsuffix /vcache_stats.csv,$(TESTS))
	@echo Creating $@
	@PYTHONPATH=$(EXAMPLES_PATH)/cuda/bfs/hammerblade-helpers/py python3 $(EXAMPLES_PATH)/cuda/bfs/py/vcache.py $^

purge:
	rm -rf $(TESTS)
	rm -f input-analyze
	rm -f input-gen
	rm -rf inputs


