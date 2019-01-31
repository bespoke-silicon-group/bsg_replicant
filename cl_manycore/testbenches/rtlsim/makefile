# Amazon FPGA Hardware Development Kit
#
# Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Amazon Software License (the "License"). You may not use
# this file except in compliance with the License. A copy of the License is
# located at
#
#    http://aws.amazon.com/asl/
#
# or in the "license" file accompanying this file. This file is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
# implied. See the License for the specific language governing permissions and
# limitations under the License.
ifndef CL_DIR
$(error "CL_DIR is not defined. Set CL_DIR to the root of this project")
endif

ifndef HDK_COMMON_DIR
$(error "HDK variables undefined. Run `source hdk_setup.sh` in aws-fpga")
endif

ifndef PROJECT
$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif

# AWS Paths (Don't modify)
export XILINX_IP    = $(HDK_SHELL_DESIGN_DIR)/ip
SDK_DIR             = $(AWS_FPGA_REPO_DIR)/sdk
C_COMMON_DIR        = $(HDK_COMMON_DIR)/software
C_SDK_USR_INC_DIR   = $(SDK_DIR)/userspace/include
C_SDK_USR_UTILS_DIR = $(SDK_DIR)/userspace/utils

# Simulation Variables (Modify if directory structure changes)
RTL_TB_MODULE = cl_tb
SCRIPTS_DIR   = $(CL_DIR)/testbenches/rtlsim
# SIM_ROOT used by included makefiles
SIM_ROOT      = $(CL_DIR)/testbenches/sim/rtlsim

# Cosim C-File Variables (Modify C_TB_TOP if the top file is not $(PROJECT).c
C_INC_DIR = $(CL_DIR)/software/include
C_SRC_DIR = $(CL_DIR)/software
# C_TB_TOP names the software C file that drives cosimulation
C_TB_TOP  = $(HDK_COMMON_DIR)/software/src/test_null.c
C_FILES   = $(C_TB_TOP) $(C_SDK_USR_UTILS_DIR)/sh_dpi_tasks.c \
		$(C_COMMON_DIR)/src/fpga_pci_sv.c 

# SV TEST LIST used by included makefiles (Purpose unclear)
SV_TEST_LIST = $(PROJECT)

ifdef CHECK
XVLOG_OPTS +=-d ENABLE_PROTOCOL_CHK
XELOG_OPTS +=$(XVLOG_OPTS)
endif

ifdef DEBUG
XSIM_OPTS +=-gui
export GUI
endif

run: make_sim_dir compile sim

include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
