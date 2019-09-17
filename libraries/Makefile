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

build: $(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0

install: $(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0
	mv $(notdir $^) /usr/lib64/
	ln -sf /usr/lib64/$(notdir $^) /usr/lib64/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib64/$(notdir $^) /usr/lib64/libbsg_manycore_runtime.so
	cp -t /usr/include $(HEADERS)

clean: libraries.clean

uninstall: clean
	sudo rm -f /usr/lib64/libbsg_manycore_* /usr/include/bsg_manycore*.h

.PHONY: build install clean uninstall help

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {build|install|uninstall|clean}"
	@echo "      build: Compile the BSG Manycore Runtime Libraries"
	@echo "             (libbsg_manycore_runtime.so)"
	@echo "      install: Install libbsg_manycore_runtime.so in"
	@echo "             /usr/lib64 and the headers in /usr/include"
	@echo "      uninstall: Remove the installed libraries"
	@echo "      clean: Remove all subdirectory-specific outputs"
