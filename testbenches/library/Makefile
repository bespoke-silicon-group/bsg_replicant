# This (rather sparse) Makefile is responsible for building and
# running regression tests in cosimulation.  Run `make help` to see the
# available targets. (The target `help` is defined in cosimulation.mk)

# environment.mk verifies the build environment and sets the following
# makefile variables:
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../../environment.mk

# The following variables are set by $(CL_DIR)/hdk.mk
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(CL_DIR)/hdk.mk

# tests.mk defines all of the regression tests (and the source path)
# for this sub-directory.
include $(REGRESSION_PATH)/library/tests.mk

# The HDK Makefile must come before cosimulation.mk to override AWS'
# (broken) clean HDK_COMMON_DIR is set by running `source
# hdk_setup.sh` from the aws-fpga repository
include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc

# REGRESSION_TESTS_TYPE is defined by tests.mk in each regression
# sub-directory.
SIM_PATH=$(TESTBENCH_PATH)/$(REGRESSION_TESTS_TYPE)/

# cosimulation.mk defines all of the rules for building cosimulation
# binaries
include $(TESTBENCH_PATH)/cosimulation.mk

# rules.mk defines rules for building RISC-V binaries associated with
# the tests in this sub-directory. rules.mk must come after
# cosimulation.mk so that <test_name>.rule and <test_name>.clean are
# not overwritten.
include $(REGRESSION_PATH)/$(REGRESSION_TESTS_TYPE)/rules.mk
