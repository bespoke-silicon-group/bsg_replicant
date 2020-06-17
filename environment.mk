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

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

ifndef __BSG_ENVIRONMENT_MK
__BSG_ENVIRONMENT_MK := 1

# Name of this project
PROJECT = cl_manycore

CL_DIR           := $(shell git rev-parse --show-toplevel)
HARDWARE_PATH    := $(CL_DIR)/hardware
LIBRARIES_PATH   := $(CL_DIR)/libraries
MACHINES_PATH    := $(CL_DIR)/machines
EXAMPLES_PATH    := $(CL_DIR)/examples

# Check if we are running inside of the BSG Bladerunner repository by searching
# for project.mk. If project.mk is found, then we are and we should use
# that to define BASEJUMP_STL_DIR, and BSG_MANYCORE_DIR and
# override previous settings. Warn, to provide feedback.
ifneq ("$(wildcard $(CL_DIR)/../project.mk)","")

# Override BASEJUMP_STL_DIR, and BSG_MANYCORE_DIR.
# If they were previously set, warn if there is a mismatch between the
# new value and the old (temporary). Using := is critical here since
# it assigns the value of the variable immediately.

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

# Include project.mk from Bladerunner. This will override
# BASEJUMP_STL_DIR, and BSG_MANYCORE_DIR
include $(CL_DIR)/../project.mk

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
endif # Matches: ifneq ("$(wildcard $(CL_DIR)/../project.mk)","")

# If BASEJUMP_STL_DIR is not defined at this point, raise an error.
ifndef BASEJUMP_STL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BASEJUMP_STL_DIR environment variable undefined. Defining is not recommended. Are you running from within Bladerunner?$(NC)"))
endif

# If BSG_MANYCORE_DIR is not defined at this point, raise an error.
ifndef BSG_MANYCORE_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MANYCORE_DIR environment variable undefined. Defining is not recommended. Are you running from within Bladerunner?$(NC)"))
endif

# cadenv.mk defines the CAD environment (for BSG people)
include $(CL_DIR)/cadenv.mk

# machine.mk defines BSG_MACHINE_PATH, which is the path to the target machine
include $(CL_DIR)/machine.mk

# platform.mk defines BSG_PLATFORM_PATH, which is the host platform to
# simulate (VCS or Verilator) or run on (AWS)
include $(CL_DIR)/platform.mk

endif
