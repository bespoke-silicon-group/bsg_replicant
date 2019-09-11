# This makefile fragment configures the AWS-FPGA HDK Environment. We
# do this so that users do NOT need to run `source hdk_setup.sh` from
# the AWS-FPGA repository and dirty their environment
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This Makefile requires environment.mk to be included by a previous
# makefile
ifndef __MAKEFILE_ENVIRONMENT
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: environment.mk not previously included$(NC)"))
endif

# Double check that CL_DIR has been defined. It is set in
# environment.mk, but it is a critical variable so we make extra
# certain before proceeding.
ifndef CL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: CL_DIR undefined while trying to configure HDK Environment. Did you include environment.mk?$(NC)")
endif

# First, check to see if HDK_SHELL_DESIGN_DIR is defined. When `source
# hdk_setup.sh` is run HDK_SHELL_DESIGN_DIR is set.
# HDK_SHELL_DESIGN_DIR depends on HDK_DIR, HDK_COMMON_DIR, and
# AWS_FPGA_REPO_DIR, so we assume that if it is set, the rest are set
# and we DO NOT need to configure the environment.
ifdef HDK_SHELL_DESIGN_DIR
$(info BSG MAKE INFO: HDK_SHELL_DESIGN_DIR is defined as: $(HDK_SHELL_DESIGN_DIR))
$(info BSG MAKE INFO: Assuming remaining HDK variables are set)

# Else, if HDK_SHELL_DESIGN_DIR is not defined, then check to see if we are
# running inside of a bladerunner-like environment, where aws-fpga is relative
# to CL_DIR. If it is, then use it to configure our environment
else ifneq ("$(wildcard $(CL_DIR)/../aws-fpga)","")
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Using $(CL_DIR)/../aws-fpga as AWS_FPGA_REPO_DIR variable$(NC)"))

AWS_FPGA_REPO_DIR    = $(CL_DIR)/../aws-fpga
HDK_DIR              = $(AWS_FPGA_REPO_DIR)/hdk
HDK_COMMON_DIR       = $(HDK_DIR)/common
HDK_SHELL_DIR        = $(realpath $(HDK_COMMON_DIR)/shell_stable/)
HDK_SHELL_DESIGN_DIR = $(HDK_SHELL_DIR)/design
SDK_DIR              = $(AWS_FPGA_REPO_DIR)/sdk
SDACCEL_DIR          = $(AWS_FPGA_REPO_DIR)/SDAccel

# Otherwise, we're screwed. Throw an error and exit
else # Matches: ifneq ("$(wildcard $(CL_DIR)/../aws-fpga)","")
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HDK variables undefined and aws-fpga directory not found. Run \`source hdk_setup.sh\` in aws-fpga.$(NC)"))
endif

# Set a few remaining variables for the HDK Environment
XILINX_IP           ?= $(HDK_SHELL_DESIGN_DIR)/ip
SDK_DIR             ?= $(AWS_FPGA_REPO_DIR)/sdk
C_COMMON_DIR        ?= $(HDK_COMMON_DIR)/software
C_SDK_USR_INC_DIR   ?= $(SDK_DIR)/userspace/include
C_SDK_USR_UTILS_DIR ?= $(SDK_DIR)/userspace/utils



