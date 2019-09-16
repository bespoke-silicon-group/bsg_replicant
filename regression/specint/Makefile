# This (rather sparse) Makefile is responsible for building and
# running regression tests on F1.  Run `make help` to see the
# available targets. (The target `help` is defined in compilation.mk

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

# tests.mk defines all of the regression tests (and the source path)
# for this sub-directory.
include tests.mk

# compillation.mk defines rules for compilation of the C/C++
# regression tests.
include $(REGRESSION_PATH)/compilation.mk

# rules.mk defines rules for building RISC-V binaries associated with
# the tests in this sub-directory. rules.mk must come after
# cosimulation.mk so that <test_name>.rule and <test_name>.clean are
# not overwritten.
include rules.mk
