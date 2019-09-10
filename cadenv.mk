# This Makefile fragment is for setting up the CAD environment for cosimulation
#
# This Makefile requires environment.mk to be included by a previous makefile
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m
ifndef __MAKEFILE_ENVIRONMENT
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: environment.mk not previously included$(NC)"))
endif

# Cosimulation requires VCS-MX and Vivado. Search for bsg_cadenv and use
# cadenv.mk to configure EDA Environment if it is present. If it is not present,
# check for Vivado and VCS.
ifneq ("$(wildcard $(CL_DIR)/../bsg_cadenv/cadenv.mk)","")
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Found bsg_cadenv. Including cadenv.mk to configure cad environment.$(NC)"))
include $(CL_DIR)/../bsg_cadenv/cadenv.mk
export VCS_HOME=$(VCSMX_HOME)
else ifndef VCS_HOME
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: VCS_HOME environment variable undefined. Are you sure vcs-mx is installed?$(NC)"))
endif

ifndef XILINX_VIVADO
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: XILINX_VIVADO environment variable undefined. Are you sure Vivado is installed?$(NC)"))
endif
