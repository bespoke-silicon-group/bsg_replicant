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

# This Makefile fragment defines the rules that are used for executing
# applications on HammerBlade Platforms

.PRECIOUS: exec.log saifgen.log
.PHONY: platform.execution.clean

RISCV_DEFINES += -Dbsg_tiles_X=$(BSG_MACHINE_POD_TILES_X)
RISCV_DEFINES += -Dbsg_tiles_Y=$(BSG_MACHINE_POD_TILES_Y)
include $(BSG_F1_DIR)/examples/cuda/riscv.mk

%.log: ZYNQPARROT_EXECUTION_DIR ?= $(ZYNQPARROT_DIR)/cosim/hammerblade-example/vcs
%.log: loader.o clr.riscv
	$(MAKE) -C $(ZYNQPARROT_EXECUTION_DIR) clean
	cp clr.riscv $(ZYNQPARROT_EXECUTION_DIR)/clr.riscv32
	cp loader.o  $(ZYNQPARROT_EXECUTION_DIR)/loader.riscv64
	$(MAKE) -C $(ZYNQPARROT_EXECUTION_DIR) \
		NUM_MC_FINISH=0 \
		NUM_BP_FINISH=1 \
		BSG_REPLICANT_DIR=$(BSG_F1_DIR) \
		BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
		BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
		NBF_FILE=clr.loader.nbf \
		run

platform.execution.clean:
	rm -rf exec.log

execution.clean: platform.execution.clean

