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

# This Makefile is for building FPGA Design Checkpoints to upload to AWS
#
# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
# PROJECT: The name of this project
include ../environment.mk

# Set the machine path so that we don't try to compile an
# unsynthesizable machine.
BSG_MACHINE_PATH := $(MACHINE_DIR)/4x4_blocking_vcache_f1_model

# The following variables are set by $(CL_DIR)/hdk.mk, which will fail if
# hdk_setup.sh has not been run, or environment.mk is not included
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(CL_DIR)/hdk.mk

# $(HARDWARE_PATH)/hardware.mk adds to VSOURCES which is a list of verilog
# source files for cosimulation and compilation, and VHEADERS, which is similar,
# but for header files. 
#
# hardware.mk also sets CL_TOP_MODULE, the top module of this design
include $(HARDWARE_PATH)/hardware.mk

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly. environment.mk must be included before this line
include $(CL_DIR)/cadenv.mk

# Replace any xilinx-unsynthesizable sources with xilinx-synthesizable sources
BUILD_SOURCES := $(filter-out $(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v,$(VHEADERS) $(VSOURCES)) 
BUILD_SOURCES += $(BASEJUMP_STL_DIR)/hard/ultrascale_plus/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v

SOURCES = $(addprefix src_post_encryption/, $(notdir $(BUILD_SOURCES)))

$(SOURCES): src_post_encryption $(BUILD_SOURCES)
	@cp $(filter %/$(notdir $@), $(BUILD_SOURCES)) $@

src_post_encryption:
	mkdir $@

build: checkpoints/to_aws/$(PROJECT).Developer_CL.tar
checkpoints/to_aws/$(PROJECT).Developer_CL.tar: $(SOURCES)
	XILINX_IP=$(XILINX_IP) \
	HDK_COMMON_DIR=$(HDK_COMMON_DIR) \
	HDK_SHELL_DESIGN_DIR=$(HDK_SHELL_DESIGN_DIR) \
	HDK_SHELL_DIR=$(HDK_SHELL_DIR) \
	HDK_DIR=$(HDK_DIR) \
	XILINX_VIVADO=$(XILINX_VIVADO) \
	CL_TOP_MODULE=$(CL_TOP_MODULE) \
	CL_DIR=$(CL_DIR) \
	./aws_build_dcp_from_cl.sh -foreground -design $(PROJECT)

clean: hardware.clean
	rm -rf *.log *.jou *.out hd_visual *.tmp 
	rm -rf checkpoints last_log .Xil .cache src_post_encryption/ reports/
	rm -rf src_post_encryption

.DEFAULT_GOAL=help
.PHONY: clean build help
help:
	@echo "Usage:"
	@echo "make {build|clean}"
	@echo "      build: Compile the AWS FPGA Design Checkpoint into"
	@echo "             ./checkpoints/to_aws/$(PROJECT).Developer_CL.tar"
	@echo "      clean: Remove all build files"
