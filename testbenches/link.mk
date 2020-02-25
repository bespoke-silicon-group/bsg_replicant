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

# This Makefile fragment defines all of the rules for linking
# cosimulation binaries

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically set by the
# Makefile that includes this fragment...

# SRC_PATH: The path to the directory containing the .c or cpp test files
ifndef SRC_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: SRC_PATH is not defined$(NC)"))
endif

# EXEC_PATH: The path to the directory where tests will be executed
ifndef EXEC_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: EXEC_PATH is not defined$(NC)"))
endif

# CL_DIR: The path to the root of the BSG F1 Repository
ifndef CL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: CL_DIR is not defined$(NC)"))
endif

# TESTBENCH_PATH: The path to the testbenches folder in BSG F1
ifndef TESTBENCH_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: TESTBENCH_PATH is not defined$(NC)"))
endif

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly.
include $(CL_DIR)/cadenv.mk

# simlibs.mk defines build rules for hardware and software simulation libraries
# that are necessary for running cosimulation. These are dependencies for
# regression since running $(MAKE) recursively does not prevent parallel builds
# of identical rules -- which causes errors.
include $(TESTBENCH_PATH)/simlibs.mk

# -------------------- VARIABLES --------------------
# We parallelize VCS compilation, but we leave a few cores on the table.
NPROCS   = $(shell echo "(`nproc`/4 + 1)" | bc)

LDFLAGS += -lbsg_manycore_runtime -lm
# libbsg_manycore_runtime will be compiled in $(LIBRARIES_PATH)
LDFLAGS += -L$(LIBRARIES_PATH) -Wl,-rpath=$(LIBRARIES_PATH)

VCS_LDFLAGS    += $(foreach def,$(LDFLAGS),-LDFLAGS "$(def)")
VCS_VFLAGS     += -M +lint=TFIPC-L -ntb_opts tb_timescale=1ps/1ps -lca -v2005 \
                -timescale=1ps/1ps -sverilog -full64 -licqueue

# VCS Generates an executable file by linking the $(SRC_PATH)/%.o file with the
# the VCS work libraries for the design, and the runtime shared libraries
$(EXEC_PATH)/%.debug: VCS_VFLAGS += -debug_pp 
$(EXEC_PATH)/%.debug: VCS_VFLAGS += +plusarg_save +vcs+vcdpluson +vcs+vcdplusmemon +memcbk

$(EXEC_PATH)/%.debug $(EXEC_PATH)/%: $(SRC_PATH)/%.o $(SIMLIBS)
	SYNOPSYS_SIM_SETUP=$(TESTBENCH_PATH)/synopsys_sim.setup \
	vcs tb glbl cosim_wrapper -j$(NPROCS) $< $(VCS_LDFLAGS) $(VCS_VFLAGS) \
		-Mdirectory=$@.tmp -o $@ -l $@.vcs.log

link.clean: 
	rm -rf $(EXEC_PATH)/DVEfiles
	rm -rf $(EXEC_PATH)/*.daidir $(EXEC_PATH)/*.tmp
	rm -rf $(EXEC_PATH)/64 $(EXEC_PATH)/.cxl*
	rm -rf $(EXEC_PATH)/*.vcs.log $(EXEC_PATH)/*.jou
	rm -rf $(EXEC_PATH)/*.key $(EXEC_PATH)/*.vpd
	rm -rf $(EXEC_PATH)/vc_hdrs.h
	rm -rf .vlogansetup* stack.info*
