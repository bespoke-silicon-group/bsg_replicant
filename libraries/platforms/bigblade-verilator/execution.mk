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

.PRECIOUS: threaded.log exec.log profile.log trace.log debug.log pc-histogram.log debug.fst
.PHONY: platform.execution.clean dve

threaded.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/threaded/simsc
debug.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/debug/simsc
exec.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/exec/simsc
profile.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/profile/simsc
trace.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/trace/simsc
pc-histogram.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/pc-histogram/simsc

%.log: SHELL := /bin/bash
%.log: main.so $(BSG_MANYCORE_KERNELS)
	@set -o pipefail; $(filter %/simsc, $^) $(CURDIR)/main.so $(C_ARGS) 2>&1 | tee $@

vanilla_stats.csv vcache_stats.csv router_stat.csv: profile.log

debug.fst: debug.log ;

platform.execution.clean:
	rm -rf saifgen.log exec.log profile.log trace.log debug.log debug.fst
	rm -rf vanilla_stats.csv
	rm -rf infinite_mem_stats.csv
	rm -rf vcache_stats.csv
	rm -rf vanilla_operation_trace.csv
	rm -rf operation_trace.csv
	rm -rf vcache_operation_trace.csv
	rm -rf router_stat.csv
	rm -rf remote_load_trace.csv
	rm -rf vanilla.log
	rm -rf debug.fst debug.fst.hier
	rm -rf ucli.key
	rm -rf dramsim3.json dramsim3.tag.json dramsim3.txt dramsim3epoch.json

execution.clean: platform.execution.clean

help:
	@echo "Usage:"
	@echo "make {sim-exec | sim-profile | sim-trace | sim-debug} (build model only)"
	@echo "make {clean | exec.log | profile.log | trace.log | debug.log | debug.fst} (run application)"
	@echo "      exec.log: Run program with SAIF, profilers, and waveform generation disabled (Fastest)"
	@echo "      profile.log: Core/cache counters and PC histogram; no instruction-text log or waveforms"
	@echo "      trace.log: Profiling plus detailed vanilla.log instruction text; no waveforms"
	@echo "      Operation CSV traces in profile/trace require runtime trace enablement"
	@echo "      Use separate application/run directories: CSVs, vanilla.log and DRAM outputs use the simulator cwd"
	@echo "      debug.log debug.fst: Run program with waveform generate enabled"
	@echo "      clean: Remove all subdirectory-specific outputs"
