ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

ifndef CL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: CL_DIR undefined while trying to configure HDK Environment. Did you include environment.mk?$(NC)")
endif

# First, check to see if HDK_SHELL_DESIGN_DIR is defined. When hdk_setup.sh is
# sourced. HDK_SHELL_DESIGN_DIR depends HDK_DIR, HDK_COMMON_DIR, and
# AWS_FPGA_REPO_DIR, so we assume that if it is set, the rest are set.
ifdef HDK_SHELL_DESIGN_DIR
$(info BSG MAKE INFO: HDK_SHELL_DESIGN_DIR is defined as: $(HDK_SHELL_DESIGN_DIR))
$(info BSG MAKE INFO: Assuming remaining HDK variables are set)

# Else, if HDK_SHELL_DESIGN_DIR is not defined, then check to see if we are
# running inside of a bladerunner-like environment, where aws-fpga is relative
# to CL_DIR. If it is, then use it to configure our environment
else ifneq ("$(wildcard $(CL_DIR)/../../aws-fpga)","")
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Using $(CL_DIR)/../../aws-fpga as AWS_FPGA_REPO_DIR variable$(NC)"))

AWS_FPGA_REPO_DIR    = $(CL_DIR)/../../aws-fpga
HDK_DIR              = $(AWS_FPGA_REPO_DIR)/hdk
HDK_COMMON_DIR       = $(HDK_DIR)/common
HDK_SHELL_DIR        = $(realpath $(HDK_COMMON_DIR)/shell_stable/)
HDK_SHELL_DESIGN_DIR = $(HDK_SHELL_DIR)/design
SDK_DIR              = $(AWS_FPGA_REPO_DIR)/sdk
SDACCEL_DIR          = $(AWS_FPGA_REPO_DIR)/SDAccel

# Otherwise, we're screwed.
else
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HDK variables undefined and aws-fpga directory not found. Run \`source hdk_setup.sh\` in aws-fpga.$(NC)"))
endif

# AWS Paths (Don't modify - HDK_SHELL_DESIGN_DIR, AWS_FPGA_REPO_DIR,
# HDK_COMMON_DIR, and SDK_DIR variables are set by hdk_setup.sh in aws-fpga)
XILINX_IP           = $(HDK_SHELL_DESIGN_DIR)/ip
SDK_DIR             = $(AWS_FPGA_REPO_DIR)/sdk
C_COMMON_DIR        = $(HDK_COMMON_DIR)/software
C_SDK_USR_INC_DIR   = $(SDK_DIR)/userspace/include
C_SDK_USR_UTILS_DIR = $(SDK_DIR)/userspace/utils



