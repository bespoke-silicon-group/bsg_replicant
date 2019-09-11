# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
#
# This file make 
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# Set __MAKEFILE_ENVIRONMENT so that others can check if this makefile
# has been included previously.
__MAKEFILE_ENVIRONMENT := 1

CL_DIR := $(shell git rev-parse --show-toplevel)
HARDWARE_PATH    = $(CL_DIR)/hardware/
REGRESSION_PATH  = $(CL_DIR)/regression/
TESTBENCH_PATH   = $(CL_DIR)/testbenches/
LIBRARIES_PATH   = $(CL_DIR)/libraries/
BSG_MACHINE_PATH = $(CL_DIR)

# Check if we are running inside of the BSG Bladerunner repository by searching
# for Makefile.common. If Makefile.common is found, then we are and we should use
# that to define BSG_IP_CORES_DIR/BASEJUMP_STL_DIR, and BSG_MANYCORE_DIR and
# override previous settings. Warn, to provide feedback.
ifneq ("$(wildcard $(CL_DIR)/../Makefile.common)","")

# Override BSG_IP_CORES_DIR, BASEJUMP_STL_DIR, and BSG_MANYCORE_DIR.
# If they were previously set, warn if there is a mismatch between the
# new value and the old (temporary). Using := is critical here since
# it assigns the value of the variable immediately.

# If BSG_IP_CORES_DIR is set, save it to a temporary variable to check
# against what is set by Bladrunner and undefine it
ifdef BSG_IP_CORES_DIR
_BSG_IP_CORES_DIR := $(BSG_IP_CORES_DIR)
undefine BSG_IP_CORES_DIR
endif

# If BASEJUMP_STL_DIR is set, save it to a temporary variable to check
# against what is set by Bladrunner and undefine it
ifdef BASEJUMP_STL_DIR
_BASEJUMP_STL_DIR := $(BASEJUMP_STL_DIR)
undefine BASEJUMP_STL_DIR
endif

# If BSG_MANYCORE_DIR is set, save it to a temporary variable to check
# against what is set by Bladrunner and undefine it
ifdef BSG_MANYCORE_DIR
_BSG_MANYCORE_DIR := $(BSG_MANYCORE_DIR)
undefine BSG_MANYCORE_DIR
endif

# Include Makefile.common from Bladerunner. This will override
# BSG_IP_CORES_DIR, BASEJUMP_STL_DIR, and BSG_MANYCORE_DIR
include $(CL_DIR)/../Makefile.common

ifdef _BSG_IP_CORES_DIR
ifneq ($(_BSG_IP_CORES_DIR), $(BSG_IP_CORES_DIR))
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Overriding BSG_IP_CORES_DIR environment variable with Bladerunner defaults.$(NC)"))
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: BSG_IP_CORES_DIR=$(BSG_IP_CORES_DIR)$(NC)"))
endif # Matches: ifneq ($(_BSG_IP_CORES_DIR), $(BSG_IP_CORES_DIR))
endif # Matches: ifdef _BSG_IP_CORES_DIR
# Undefine the temporary variable to prevent its use
undefine _BSG_IP_CORES_DIR

ifdef _BASEJUMP_STL_DIR
ifneq ($(_BASEJUMP_STL_DIR), $(BASEJUMP_STL_DIR))
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Overriding BASEJUMP_STL_DIR environment variable with Bladerunner defaults.$(NC)"))
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR)$(NC)"))
endif # Matches: ifneq ($(_BASEJUMP_STL_DIR), $(BASEJUMP_STL_DIR))
endif # Matches: ifdef _BASEJUMP_STL_DIR
# Undefine the temporary variable to prevent its use
undefine _BASEJUMP_STL_DIR

ifdef _BSG_MANYCORE_DIR
ifneq ($(_BSG_MANYCORE_DIR), $(BSG_MANYCORE_DIR))
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: Overriding BSG_MANYCORE_DIR environment variable with Bladerunner defaults.$(NC)"))
$(warning $(shell echo -e "$(ORANGE)BSG MAKE WARN: BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR)$(NC)"))
endif # Matches: ifneq ($(_BSG_MANYCORE_DIR), $(BSG_MANYCORE_DIR))
endif # Matches: ifdef _BSG_MANYCORE_DIR
# Undefine the temporary variable to prevent its use
undefine _BSG_MANYCORE_DIR
endif # Matches: ifneq ("$(wildcard $(CL_DIR)/../Makefile.common)","")

# If BSG_IP_CORES_DIR is not defined at this point, raise an error.
ifndef BSG_IP_CORES_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_IP_CORES_DIR environment variable undefined. Defining is not recommended. Are you running from within Bladerunner?$(NC)"))
endif

# If BASEJUMP_STL_DIR is not defined at this point, raise an error.
ifndef BASEJUMP_STL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BASEJUMP_STL_DIR environment variable undefined. Defining is not recommended. Are you running from within Bladerunner?$(NC)"))
endif

# If BSG_MANYCORE_DIR is not defined at this point, raise an error.
ifndef BSG_MANYCORE_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MANYCORE_DIR environment variable undefined. Defining is not recommended. Are you running from within Bladerunner?$(NC)"))
endif
