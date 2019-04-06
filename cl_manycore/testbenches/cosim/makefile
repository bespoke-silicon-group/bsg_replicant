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
RTL_TB_MODULE = cosim_wrapper
SCRIPTS_DIR   = $(CL_DIR)/testbenches/cosim/
# SIM_ROOT used by included makefiles
SIM_ROOT      = $(CL_DIR)/testbenches/sim/cosim/ 

# Cosim C-File Variables (Modify C_TB_TOP if the top file is not $(PROJECT).c
C_INC_DIR = $(CL_DIR)/software
C_SRC_DIR = $(CL_DIR)/software
# C_TB_TOP names the software C file that drives cosimulation
# C_TB_TOP  = $(C_SRC_DIR)/$(PROJECT).c
# C_TB_TOP  = $(C_SRC_DIR)/read_write_test.c
C_FILES   = $(C_TB_TOP) $(C_SDK_USR_UTILS_DIR)/sh_dpi_tasks.c \
			$(C_COMMON_DIR)/src/fpga_pci_sv.c \
			$(CL_DIR)/libraries/bsg_manycore_driver.c \
			$(CL_DIR)/libraries/bsg_manycore_loader.c \
			$(CL_DIR)/libraries/bsg_manycore_mem.c 
# SV TEST LIST used by included makefiles (Purpose unclear)
SV_TEST_LIST = $(PROJECT) 

ifdef CHECK
XVLOG_OPTS +=-d ENABLE_PROTOCOL_CHK 
XELOG_OPTS +=$(XVLOG_OPTS)
endif
ifdef DEBUG
XSIM_OPTS +=-gui
endif
ifdef AXI_MEMORY_MODEL
XVLOG_OPTS +=-d AXI_MEMORY_MODEL=1 -d ECC_DIRECT_EN -d RND_ECC_EN -d ECC_ADDR_LO=0 -d ECC_ADDR_HI=0 -d RND_ECC_WEIGHT=0
endif
run: make_sim_dir compile sim

include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
