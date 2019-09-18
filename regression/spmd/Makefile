# This (rather sparse) Makefile is responsible for building and
# running regression tests on F1.  Run `make help` to see the
# available targets. (The target `help` is defined in f1.mk)

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

# tests.mk defines all of the regression tests (and the source path) for this
# sub-directory. It defines the variable REGRESSION_TESTS_TYPE
include tests.mk

# EXEC_PATH defines this directory, where the tests are actually
# executed. REGRESSION_TESTS_TYPE is defined by tests.mk in each regression
# sub-directory. It is used in f1.mk
EXEC_PATH=$(REGRESSION_PATH)/$(REGRESSION_TESTS_TYPE)

# f1.mk defines all of the re-used rules for execution on F1. (e.g. regression,
# clean, help, etc)
include $(REGRESSION_PATH)/f1.mk

# rules.mk defines rules for building RISC-V binaries associated with the tests
# in this sub-directory. rules.mk must come after f1.mk so that
# <test_name>.rule and <test_name>.clean are not overwritten.
include rules.mk

# The rule below defines how to run test_loader for SPMD tests. The loader
# requires two arguments: The path to the RISC-V Binary loaded onto the manycore
# (TEST_PATH), and the test name (TEST_NAME). SPMD_PATH is defined for convenience
SPMD_PATH=$(BSG_MANYCORE_DIR)/software/spmd
$(EXEC_PATH)/%.log: TEST_NAME=$(subst .log,,$(notdir $@))
$(EXEC_PATH)/%.log: TEST_PATH=$(SPMD_PATH)/$(subst test_,,$(TEST_NAME))/main.riscv
$(EXEC_PATH)/%.log: $(EXEC_PATH)/test_loader %.rule
	sudo $< $(TEST_PATH) $(TEST_NAME) | tee $@
