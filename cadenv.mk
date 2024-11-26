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

# This Makefile fragment sets up the CAD environment for cosimulation
# and bitstream generation.
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# Don't include more than once
ifndef (_BSG_REPLICANT_CADENV_MK)
_BSG_REPLICANT_CADENV_MK := 1

# CL_DIR: The path to the root of the BSG F1 Repository
ifndef CL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: CL_DIR is not defined$(NC)"))
endif

# Cosimulation requires VCS-MX and Vivado. Bespoke Silicon Group uses
# bsg_cadenv to reduce environment mis-configuration errors. We simply
# check to see if bsg_cadenv exists, and use cadenv.mk to configure
# EDA Environment if it is present. If it is not present, we check for
# Vivado and VCS and warn if they do not exist.
ifneq ("$(wildcard $(CL_DIR)/../bsg_cadenv/cadenv.mk)","")
include $(CL_DIR)/../bsg_cadenv/cadenv.mk
# We use vcs-mx, so we re-define VCS_HOME in the environment
export VCS_HOME=$(VCSMX_HOME)
else
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Couldn't find bsg_cadenv. User must configure CAD Environment.$(NC)"))
endif
endif
