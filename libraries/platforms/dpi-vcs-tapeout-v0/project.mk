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

DEPENDENCIES           := bsg_manycore bsg_replicant basejump_stl

BLADERUNNER_ROOT       := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))/../../../..)))
BUILD_PATH             := $(BLADERUNNER_ROOT)

BSG_F1_DIR             := $(BLADERUNNER_ROOT)/bsg_replicant
BSG_F1_COMMIT_ID       := $(shell cd $(BSG_F1_DIR); git rev-parse --short HEAD)
BSG_MANYCORE_DIR       := $(BSG_F1_DIR)/hardware/bsg_manycore_tapeout_0
BSG_MANYCORE_COMMIT_ID := $(shell cd $(BSG_MANYCORE_DIR); git rev-parse --short HEAD)
BASEJUMP_STL_DIR       := $(BSG_F1_DIR)/hardware/basejump_stl_tapeout_0
BASEJUMP_STL_COMMIT_ID := $(shell cd $(BASEJUMP_STL_DIR); git rev-parse --short HEAD)

FPGA_IMAGE_VERSION     :=
F12XLARGE_TEMPLATE_ID  :=
AFI_ID                 :=
AGFI_ID                :=