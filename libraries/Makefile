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

# This Makefile contains rules for building the BSG Manycore Runtime
# libraries.

# environment.mk verifies the build environment and sets the following
# variables
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../environment.mk

# The following variables are set by $(CL_DIR)/hdk.mk
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(CL_DIR)/hdk.mk

# libraries.mk defines rules for building the BSG Manycore Runtime libraries,
# flags, and a list of sources.
include $(LIBRARIES_PATH)/libraries.mk

_TARGETS :=
_DOCSTRING :=

_DOCSTRING += "Rules from Makefile\n"
_TARGETS :=

.DEFAULT_GOAL := help
_TARGETS += "help"
_DOCSTRING += "    help: \n"
_DOCSTRING += "        - Output a friendly help message\n"
help:
	@echo -e $(DOCSTRING)

_TARGETS += "build"
_DOCSTRING += "    build:\n"
_DOCSTRING += "        - Compile the BSG Manycore Runtime Libraries (libbsg_manycore_runtime.so)\n"
_DOCSTRING += "          in BSG_PLATFORM_PATH (./$(shell realpath --relative-to . $(BSG_PLATFORM_PATH)))\n"
build: $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0

_TARGETS += "clean"
_DOCSTRING += "    clean:\n"
_DOCSTRING += "        - Remove all outputs\n"
clean: libraries.clean

.PHONY: build clean help

_TARGETS += $(TARGETS)
TARGETS := $(_TARGETS)
TARGETS := $(shell echo "$(TARGETS)" | tr -s " ")

_DOCSTRING += "\n"
_DOCSTRING += $(DOCSTRING)
DOCSTRING := "\nUsage: make {$(TARGETS)} (default: $(.DEFAULT_GOAL))\n" $(_DOCSTRING)

