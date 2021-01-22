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
# aws-vcs binaries

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically set by the
# Makefile that includes this fragment...

# BSG_PLATFORM_PATH: The path to the testbenches folder in BSG F1
ifndef BSG_PLATFORM_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_PLATFORM_PATH is not defined$(NC)"))
endif

# hardware.mk is the file list for the simulation RTL. It includes the
# platform specific hardware.mk file.
include $(HARDWARE_PATH)/hardware.mk

# libraries.mk defines how to build libbsg_manycore_runtime.so, which is
# pre-linked against all other simulation binaries.
include $(LIBRARIES_PATH)/libraries.mk

# VHEADERS must be compiled before VSOURCES.
VDEFINES += BSG_MACHINE_ORIGIN_X_CORD=$(BSG_MACHINE_ORIGIN_COORD_X)
VDEFINES += BSG_MACHINE_ORIGIN_Y_CORD=$(BSG_MACHINE_ORIGIN_COORD_Y)
VDEFINES += HOST_MODULE_PATH=replicant_tb_top
VLOGAN_SOURCES  += $(VHEADERS) $(VSOURCES)
VLOGAN_INCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VLOGAN_DEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VLOGAN_VFLAGS   += -ntb_opts tb_timescale=1ps/1ps -timescale=1ps/1ps
VLOGAN_VFLAGS   += -sverilog -v2005 +v2k
VLOGAN_VFLAGS   += +systemverilogext+.svh +systemverilogext+.sv
VLOGAN_VFLAGS   += +libext+.sv +libext+.v +libext+.vh +libext+.svh
VLOGAN_VFLAGS   += -full64 -lca -assert svaext
VLOGAN_VFLAGS   += -undef_vcs_macro

$(BSG_MACHINE_PATH)/$(BSG_PLATFORM):
	mkdir -p $@

$(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/synopsys_sim.setup: | $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)
	echo "replicant_tb_top : $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/vcs_simlibs/replicant_tb_top/64" >> $@;

$(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/vcs_simlibs/replicant_tb_top/AN.DB: $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/synopsys_sim.setup $(VHEADERS) $(VSOURCES)
	SYNOPSYS_SIM_SETUP=$(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/synopsys_sim.setup \
	vlogan -work replicant_tb_top $(VLOGAN_VFLAGS) $(VLOGAN_DEFINES) \
		$(VLOGAN_SOURCES) $(VLOGAN_INCLUDES) \
		-l $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/vlogan.log

# libbsg_manycore_runtime will be compiled in $(BSG_PLATFORM_PATH)
LDFLAGS        += -lbsg_manycore_runtime -lm
LDFLAGS        += -L$(BSG_PLATFORM_PATH) -Wl,-rpath=$(BSG_PLATFORM_PATH)

VCS_LDFLAGS    += $(foreach def,$(LDFLAGS),-LDFLAGS "$(def)")
VCS_VFLAGS     += -M -L -ntb_opts tb_timescale=1ps/1ps -lca -v2005
VCS_VFLAGS     += -timescale=1ps/1ps -sverilog -full64 -licqueue -q
VCS_VFLAGS     += +warn=noLCA_FEATURES_ENABLED
VCS_VFLAGS     += +warn=noMC-FCNAFTMI
VCS_VFLAGS     += +lint=all,TFIPC-L,noSVA-UA,noSVA-NSVU,noVCDE,noSVA-AECASR

# VCS Generates an executable file by linking the %.o file with the
# the VCS work libraries for the design, and the runtime shared libraries

# The % and %.debug rules are identical. They must be separate
# otherwise make doesn't match the latter target(s)
%: %.o $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/vcs_simlibs/replicant_tb_top/AN.DB $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
	SYNOPSYS_SIM_SETUP=$(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/synopsys_sim.setup \
	vcs replicant_tb_top $< $(VCS_LDFLAGS) $(VCS_VFLAGS) \
		-Mdirectory=$@.tmp -o $@ -l $@.vcs.log

%.debug: VCS_VFLAGS += -debug_pp
%.debug: VCS_VFLAGS += +plusarg_save +vcs+vcdpluson +vcs+vcdplusmemon +memcbk
%.debug: %.o $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/vcs_simlibs/replicant_tb_top/AN.DB $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so $(BSG_PLATFORM_PATH)/msg_config
	SYNOPSYS_SIM_SETUP=$(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/synopsys_sim.setup \
	vcs replicant_tb_top $< $(VCS_LDFLAGS) $(VCS_VFLAGS) \
		-Mdirectory=$@.tmp -o $@ -l $@.vcs.log

.PHONY: platform.link.clean
platform.link.clean:
	rm -rf $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/
	rm -rf ucli.key
	rm -rf .cxl* *.jou
	rm -rf *.daidir *.tmp
	rm -rf *.jou
	rm -rf *.vcs.log
	rm -rf vc_hdrs.h
	rm -rf $(BSG_PLATFORM_PATH)/msg_config
	rm -rf *.debug

link.clean: platform.link.clean ;
