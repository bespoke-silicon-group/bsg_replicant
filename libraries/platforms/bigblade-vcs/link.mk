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
VDEFINES += BSG_MACHINE_DRAMSIM3_PKG=$(BSG_MACHINE_MEM_DRAMSIM3_PKG)

# libbsg_manycore_runtime will be compiled in $(BSG_PLATFORM_PATH)
VLDFLAGS += -L$(BSG_PLATFORM_PATH) -Wl,-rpath=$(BSG_PLATFORM_PATH)
VLDFLAGS += -lbsg_manycore_regression -lbsg_manycore_runtime -lm
VCS_LDFLAGS += $(foreach def,$(VLDFLAGS),-LDFLAGS "$(def)")
VCS_VFLAGS  += -M -L -override_timescale=1ps/1ps -ntb_opts tb_timescale=1ps/1ps -lca
VCS_VFLAGS  += -timescale=1ps/1ps -sverilog -full64 -licqueue -q
VCS_VFLAGS  += -assert svaext -undef_vcs_macro
VCS_VFLAGS  += +warn=noLCA_FEATURES_ENABLED
VCS_VFLAGS  += +warn=noMC-FCNAFTMI
VCS_VFLAGS  += +lint=all,TFIPC-L,noSVA-UA,noSVA-NSVU,noVCDE,noSVA-AECASR
VCS_INCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VCS_DEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VCS_FLAGS   = $(VCS_LDFLAGS) $(VCS_VFLAGS) $(VCS_INCLUDES) $(VCS_DEFINES)
VCS_VSOURCES = $(VHEADERS) $(VSOURCES)

$(BSG_MACHINExPLATFORM_PATH)/debug/simv: VCS_VFLAGS += +plusarg_save +vcs+vcdpluson +vcs+vcdplusmemon +memcbk -debug_pp

$(BSG_MACHINExPLATFORM_PATH)/saifgen/simv $(BSG_MACHINExPLATFORM_PATH)/exec/simv: VDEFINES += BSG_MACHINE_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/saifgen/simv $(BSG_MACHINExPLATFORM_PATH)/exec/simv: VDEFINES += BSG_MACHINE_DISABLE_CACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/saifgen/simv $(BSG_MACHINExPLATFORM_PATH)/exec/simv: VDEFINES += BSG_MACHINE_DISABLE_ROUTER_PROFILING

$(BSG_MACHINExPLATFORM_PATH)/saifgen/simv: VDEFINES += BSG_MACHINE_ENABLE_SAIF
$(BSG_MACHINExPLATFORM_PATH)/saifgen/simv: VCS_VFLAGS += -debug_pp

$(BSG_MACHINExPLATFORM_PATH)/exec $(BSG_MACHINExPLATFORM_PATH)/debug $(BSG_MACHINExPLATFORM_PATH)/saifgen $(BSG_MACHINExPLATFORM_PATH)/profile:
	mkdir -p $@

%/simv: $(VCS_VSOURCES) | $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so %
	vcs -top replicant_tb_top $(VCS_VSOURCES) $(VCS_FLAGS) -Mdirectory=$@.tmp -l $@.vcs.log -o $@

.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/exec/simv
.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/debug/simv
.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/proile/simv
.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/saifgen/simv

# When running recursive regression, make is launched in independent,
# non-communicating parallel processes that try to build these objects
# in parallel. That can lead to processes stomping on each other. We
# define REGRESSION_PREBUILD so that regression tests can build them
# before launching parallel execution
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/exec/simv
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/debug/simv
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/profile/simv
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/saifgen/simv
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so

.PHONY: platform.link.clean
platform.link.clean:
	rm -rf $(BSG_MACHINExPLATFORM_PATH)
	rm -rf ucli.key
	rm -rf .cxl* *.jou
	rm -rf *.daidir *.tmp
	rm -rf *.jou
	rm -rf *.vcs.log
	rm -rf vc_hdrs.h
	rm -rf *.debug *.profile *.saifgen *.exec

link.clean: platform.link.clean ;

