# This Makefile fragment sets up the CAD environment for cosimulation
# and bitstream generation.
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This Makefile requires environment.mk to be included by a previous makefile
ifndef __MAKEFILE_ENVIRONMENT
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: environment.mk not previously included$(NC)"))
endif

# Cosimulation requires VCS-MX and Vivado. Bespoke Silicon Group uses
# bsg_cadenv to reduce environment mis-configuration errors. We simply
# check to see if bsg_cadenv exists, and use cadenv.mk to configure
# EDA Environment if it is present. If it is not present, we check for
# Vivado and VCS and warn if they do not exist.
ifneq ("$(wildcard $(CL_DIR)/../bsg_cadenv/cadenv.mk)","")
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Found bsg_cadenv. Including cadenv.mk to configure cad environment.$(NC)"))
include $(CL_DIR)/../bsg_cadenv/cadenv.mk
# We use vcs-mx, so we re-define VCS_HOME in the environment
export VCS_HOME=$(VCSMX_HOME)

else ifndef VCS_HOME # Matches: ifneq ("$(wildcard $(CL_DIR)/../bsg_cadenv/cadenv.mk)","")
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: VCS_HOME environment variable undefined. Are you sure vcs-mx is installed?$(NC)"))
endif # Matches: else ifndef VCS_HOME 

# XILINX_VIVADO is set by Vivado's configuration script. We use this
# as a quick check instead of running Vivado.
ifndef XILINX_VIVADO
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: XILINX_VIVADO environment variable undefined. Are you sure Vivado is installed?$(NC)"))
endif
