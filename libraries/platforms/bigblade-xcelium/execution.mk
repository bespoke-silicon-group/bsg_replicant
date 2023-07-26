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


SIM_ARGS +=

# These are the execution rules for the binaries. We can't pass
# C-style arguments through the command line, so instead we specify
# them as the SIMXplusarg argument +c_args. Users can specify C-style
# arguments using the C_ARGS make variable.

.PRECIOUS: saifgen.log exec.log profile.log exec.log debug.shm
.PHONY: platform.execution.clean simvision

saifgen.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/saifgen/simx
debug.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/debug/simx
exec.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/exec/simx
profile.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/profile/simx
pc-histogram.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/pc-histogram/simx
repl.log: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/repl/simx

empty :=
space := $(empty) $(empty)

# Xcelium cannot handle space separated plusargs, convert to @
%.log: main.so $(BSG_MANYCORE_KERNELS) 
	$(filter %/simx, $^) $(SIM_ARGS) +c_args="$(subst $(space),@,$(C_ARGS))" +c_path=$(CURDIR)/main.so 2>&1 | tee $@

vanilla_stats.csv vcache_stats.csv router_stat.csv: profile.log

saifgen.saif: saifgen.log ;

debug.shm: SIM_ARGS +=
debug.shm: debug.log ;

simvision: debug.shm
	$(SIMVISION) -64bit $< &

platform.execution.clean:
	rm -rf xcelium.d xrun.history
	rm -rf saifgen.log exec.log profile.log repl.log debug.shm
	rm -rf vanilla_stats.csv
	rm -rf infinite_mem_stats.csv
	rm -rf vcache_stats.csv
	rm -rf vanilla_operation_trace.csv
	rm -rf operation_trace.csv
	rm -rf vcache_operation_trace.csv
	rm -rf vanilla_core_pc_hist.csv
	rm -rf router_stat.csv
	rm -rf remote_load_trace.csv
	rm -rf vanilla.log
	rm -rf debug.shm 
	rm -rf ucli.key
	rm -rf dramsim3.json dramsim3.tag.json dramsim3.txt dramsim3epoch.json

execution.clean: platform.execution.clean

help:
	@echo "Usage:"
	@echo "make {clean | exec.log | profile.log | debug.log | debug.shm | saifgen.log | saifgen.saif }"
	@echo "      exec.log: Run program with SAIF, profilers, and waveform generation disabled (Fastest)"
	@echo "      profile.log: Run program with profilers enabled, SAIF and waveform generation disabled"
	@echo "      saifgen.log saifgen.saif: Run program with SAIF generation enabled, profilers and waveform generation disabled"
	@echo "      debug.log debug.shm: Run program with waveform and profiles enabled, SAIF generation disabled"
	@echo "      clean: Remove all subdirectory-specific outputs"
