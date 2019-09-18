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

# The rule below defines how to run test_loader for SPMD tests. The loader
# requires two arguments: The path to the RISC-V Binary loaded onto the manycore
# (TEST_PATH), and the test name (TEST_NAME). SPECINT_PATH is defined for convenience
SPECINT_PATH=$(BSG_MANYCORE_DIR)/software/spmd/specint2000
$(EXEC_PATH)/%.log: TEST_NAME=$(subst .log,,$(notdir $@))
$(EXEC_PATH)/%.log: TEST_PATH=$(SPECINT_PATH)/$(subst test_,,$(TEST_NAME))/main.riscv
$(EXEC_PATH)/%.log: #$(EXEC_PATH)/test_loader #%.rule
	sudo $< $(TEST_PATH) $(TEST_NAME) | tee $@


