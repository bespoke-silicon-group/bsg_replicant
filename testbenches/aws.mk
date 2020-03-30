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

# This Makefile fragment is for building hardware and software libraries for
# cosimulation
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 

# BSG_F1_DIR: The path to the testbenches folder in BSG F1
ifndef BSG_F1_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_F1_DIR is not defined$(NC)"))
endif

# The following variables are set by $(BSG_F1_DIR)/hdk.mk, which will fail if
# hdk_setup.sh has not been run, or environment.mk is not included
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(BSG_F1_DIR)/hdk.mk

VDEFINES += CARD_1=card

# AWS source library directories
# No encrypted source files.
VLIBS += $(HDK_SHELL_DESIGN_DIR)/lib
VLIBS += $(HDK_SHELL_DESIGN_DIR)/interfaces

# hlx/verif contains cl_ports_sh_bfm.vh, used in sh_bfm.sv
VINCLUDES += $(HDK_SHELL_DIR)/hlx/verif
VINCLUDES += $(HDK_SHELL_DESIGN_DIR)/lib
VINCLUDES += $(HDK_SHELL_DESIGN_DIR)/interfaces

# Type definitions for simulation code
# Must be compiled before remaining AWS Verilog sources
# Not Encrypted
VSOURCES += $(HDK_COMMON_DIR)/verif/tb/sv/tb_type_defines_pkg.sv

################################################################################
# SH BFM (Bus Functional model) Files
################################################################################

# Both contain encrypted files, notably sh_ddr (Encrypted) and
# axi4_slave_bfm (Not Encrypted)
VLIBS += $(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim
VINCLUDES += $(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim

# Not Encrypted
VINCLUDES += $(HDK_COMMON_DIR)/verif/models/sh_bfm
# Not encrypted, no submodules
VSOURCES += $(HDK_COMMON_DIR)/verif/models/xilinx_axi_pc/axi_protocol_checker_v1_1_vl_rfs.v
# Not encrypted, no submodules
VSOURCES += $(HDK_COMMON_DIR)/verif/models/sh_bfm/axil_bfm.sv

# sh_bfm.sv -> (axil_bfm, axil_protocol_checker, 
# (IF DDR MODEL) axi_register_slice, axi_clock_converter, ddr4_core
# (IF AXI Memory Model) axi4_slave_bfm (AXI memory model)
VSOURCES += $(HDK_COMMON_DIR)/verif/models/sh_bfm/sh_bfm.sv

################################################################################
# Top-Modules of Testbench
################################################################################

# fpga.sv contains sh_bfm and CL instantiations
# tb -> card -> fpga -> (CL, sh_bfm)
# Not Encrypted
VINCLUDES += $(HDK_COMMON_DIR)/verif/models/fpga
VSOURCES += $(HDK_COMMON_DIR)/verif/models/fpga/fpga.sv
VSOURCES += $(HDK_COMMON_DIR)/verif/models/fpga/card.sv
VSOURCES += $(HDK_COMMON_DIR)/verif/tb/sv/tb.sv
