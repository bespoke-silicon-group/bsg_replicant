# This file contains the top-level rules for interacting with this
# project. The rules are documented in the makefile-default "help"
# command

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include environment.mk

# Name of this project
PROJECT = cl_manycore
.PHONY: help build regression cosim clean

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {build|regression|cosim|clean}"
	@echo "      build: Runs Vivado and Generates the Design Checkpoint to"
	@echo "	            upload to AWS."
	@echo "      regression: *Only runs on an F1 instance* Runs all "
	@echo "             software regression tests"
	@echo "      cosim: Runs all regression tests in C/C++ Co-simulation"
	@echo "      clean: Remove all build files"

build:
	$(MAKE) -C $@ PROJECT=$(PROJECT)

regression:
	$(MAKE) -C regression $@ 

# The following variables control different behaviors in C/C++
# Co-simulation. These can be set from the command line, e.g: `make
# cosim DEBUG=1`. They are passed as variables to the Makefile in the
# testbenches directory.

# DEBUG=1 Opens the Waveform GUI during simulation

# AXI_PROT_CHECK=1 enables an AXI Protocol Checker on the PCI
# Interface of the AWS Shell

# AXI_MEMORY_MODEL=0 directs simulation to use the slower, but more
# accurate, DDR Model.

# TURBO=1 increases simulation speed, but disables waveform generation

# EXTRA_TURBO=1 further increases simulation speed, but may affect
# correctness.
DEBUG ?=
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?=
TURBO ?= 1 
EXTRA_TURBO ?= 0

cosim: 
	$(MAKE) -C testbenches regression DEBUG=$(DEBUG)	\
		AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL)		\
		AXI_PROT_CHECK=$(AXI_PROT_CHECK) TURBO=$(TURBO)	\
		EXTRA_TURBO=$(EXTRA_TURBO)

clean:
	$(MAKE) -C testbenches clean 
	$(MAKE) -C build clean 
	$(MAKE) -C regression clean
	$(MAKE) -C hardware clean 

