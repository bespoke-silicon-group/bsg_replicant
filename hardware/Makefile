# This Makefile builds all of the HDL files that are necessary for
# cosimulation and bitstream generation.

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../environment.mk

# $(HARDWARE_PATH)/hardware.mk adds to VSOURCES which is a list of verilog
# source files for cosimulation and compilation, and VHEADERS, which is similar,
# but for header files. It also adds to CLEANS, a list of clean rules for
# cleaning hardware targets.
include hardware.mk

all: $(VSOURCES) $(VHEADERS)
hardware:all
clean: $(CLEANS)

.PHONY: clean all hardware
.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {all|clean|hardware}"
	@echo "      all: Alias for the 'hardware' rule"
	@echo "      hardware: Generate all HDL source files"
	@echo "      clean: Remove all generated outputs"
