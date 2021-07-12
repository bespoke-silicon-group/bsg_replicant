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

ifndef __BSG_NETSIM_EXECUTION_MK
__BSG_NETSIM_EXECUTION_MK := 1

# EXAMPLES_PATH: The path to the execution platform
ifndef EXAMPLES_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: EXAMPLES_PATH is not defined$(NC)"))
endif

netsim.log: netsim/simv

include $(EXAMPLES_PATH)/execution.mk

NETSIM_PATH ?= $(EXAMPLES_PATH)/networksim

router_utilization.rpt: netsim.log
	python3 $(NETSIM_PATH)/router_parser.py

tile_execution.rpt: netsim.log
	python3 $(NETSIM_PATH)/tileparser.py

%.png: %.rpt
	pango-view  --font mono -qo $@ $<

cache_utilization.png: netsim.log
	python3 $(NETSIM_PATH)/cacheutil.py

vcache_stall_detailed.png: netsim.log
	PYTHONPATH=$(BSG_MANYCORE_DIR)/software/py/vanilla_parser/.. python3 -m vanilla_parser --only vcache_stall_graph --trace vcache_operation_trace.csv --generate-key

execution.clean: netsim.execution.clean
netsim.execution.clean:
	rm -rf dpi_stats.csv

endif
