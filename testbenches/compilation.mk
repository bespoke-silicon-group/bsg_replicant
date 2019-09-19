# This Makefile Fragment defines all of the rules for building
# cosimulation binaries

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 
# REGRESSION_TESTS: Names of all available regression tests.
ifndef REGRESSION_TESTS
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_TESTS is not defined$(NC)"))
endif

# SRC_PATH: The path to the directory containing the .c or cpp test files
ifndef SRC_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: SRC_PATH is not defined$(NC)"))
endif

# EXEC_PATH: The path to the directory where tests will be executed
ifndef EXEC_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: EXEC_PATH is not defined$(NC)"))
endif

# CL_DIR: The path to the root of the BSG F1 Repository
ifndef CL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: CL_DIR is not defined$(NC)"))
endif

# HARDWARE_PATH: The path to the hardware folder in BSG F1
ifndef HARDWARE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HARDWARE_PATH is not defined$(NC)"))
endif

# TESTBENCH_PATH: The path to the testbenches folder in BSG F1
ifndef TESTBENCH_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: TESTBENCH_PATH is not defined$(NC)"))
endif

# REGRESSION_PATH: The path to the regression folder in BSG F1
ifndef REGRESSION_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: REGRESSION_PATH is not defined$(NC)"))
endif

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly.
include $(CL_DIR)/cadenv.mk

# The following variables are set by $(CL_DIR)/hdk.mk
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(CL_DIR)/hdk.mk

# simlibs.mk defines build rules for hardware and software simulation libraries
# that are necessary for running cosimulation. These are dependencies for
# regression since running $(MAKE) recursively does not prevent parallel builds
# of identical rules -- which causes errors.
include $(TESTBENCH_PATH)/simlibs.mk

# -------------------- Arguments --------------------
# This Makefile has several optional "arguments" that are passed as Variables
#
# DEBUG: Opens the GUI during cosimulation. Default: 0
# TURBO: Disables VPD generation. Default: 0
# EXTRA_TURBO: Disables VPD Generation, and more optimization flags: Default 0
# 
# If you need additional speed, you can set EXTRA_TURBO=1 during compilation. 
# This is a COMPILATION ONLY option. Any subsequent runs, without compilation
# will retain this setting
DEBUG            ?= 0
TURBO            ?= 0
EXTRA_TURBO      ?= 0

# -------------------- VARIABLES --------------------
# We parallelize VCS compilation, but we leave a few cores on the table.
NPROCS = $(shell echo "(`nproc`/4 + 1)" | bc)

# Name of the cosimulation wrapper system verilog file.
WRAPPER_NAME = cosim_wrapper

# libfpga_mgmt will be compiled in $(TESTBENCH_PATH)
LDFLAGS    += -lbsg_manycore_runtime -lm
LDFLAGS    += -L$(TESTBENCH_PATH) -Wl,-rpath=$(TESTBENCH_PATH)

# libbsg_manycore_runtime will be compiled in $(LIBRARIES_PATH)
LDFLAGS    += -L$(LIBRARIES_PATH) -Wl,-rpath=$(LIBRARIES_PATH)
# The bsg_manycore_runtime headers are in $(LIBRARIES_PATH) (for cosimulation)
INCLUDES   += -I$(LIBRARIES_PATH) 

# CSOURCES/HEADERS should probably go in some regression file list.
CSOURCES   += 
CHEADERS   += $(REGRESSION_PATH)/cl_manycore_regression.h
CDEFINES   += -DCOSIM -DVCS
CXXSOURCES += 
CXXHEADERS += $(REGRESSION_PATH)/cl_manycore_regression.h
CXXDEFINES += -DCOSIM -DVCS
CXXFLAGS   += -lstdc++

VCS_CFLAGS     += $(foreach def,$(CFLAGS),-CFLAGS "$(def)")
VCS_CDEFINES   += $(foreach def,$(CDEFINES),-CFLAGS "$(def)")
VCS_INCLUDES   += $(foreach def,$(INCLUDES),-CFLAGS "$(def)")
VCS_CXXFLAGS   += $(foreach def,$(CXXFLAGS),-CFLAGS "$(def)")
VCS_CXXDEFINES += $(foreach def,$(CXXDEFINES),-CFLAGS "$(def)")
VCS_LDFLAGS    += $(foreach def,$(LDFLAGS),-LDFLAGS "$(def)")
VCS_VFLAGS     += -M +lint=TFIPC-L -ntb_opts tb_timescale=1ps/1ps -lca -v2005 \
                -timescale=1ps/1ps -sverilog -full64 -licqueue

# NOTE: undef_vcs_macro is a HACK!!! 
# `ifdef VCS is only used is in tb.sv top-level in the aws-fpga repository. This
# macro guards against generating vpd files, which slow down simulation.
ifeq ($(EXTRA_TURBO), 1)
VCS_VFLAGS    += +rad -undef_vcs_macro
else 
VCS_VFLAGS    += -debug_pp
VCS_VFLAGS    += +memcbk 
endif

ifeq ($(TURBO), 1)
SIM_ARGS += +NO_WAVES
else 
SIM_ARGS +=
endif

ifeq ($(DEBUG), 1)
VCS_VFLAGS    += -gui
VCS_VFLAGS    += -R
VCS_VFLAGS    += -debug_all
VCS_VFLAGS    += +memcbk
endif

# VCS Generates an executable file by compiling the $(SRC_PATH)/%.c or
# $(SRC_PATH)/%.cpp file that corresponds to the target test in the
# $(SRC_PATH) directory.
$(EXEC_PATH)/%: $(SRC_PATH)/%.c $(CSOURCES) $(CHEADERS) $(SIMLIBS)
	SYNOPSYS_SIM_SETUP=$(TESTBENCH_PATH)/synopsys_sim.setup \
	vcs tb glbl -j$(NPROCS) $(WRAPPER_NAME) $< -Mdirectory=$@.tmp \
		$(VCS_CFLAGS) $(VCS_CDEFINES) $(VCS_INCLUDES) $(VCS_LDFLAGS) \
		$(VCS_VFLAGS) -o $@ -l $@.vcs.log

$(EXEC_PATH)/%: $(SRC_PATH)/%.cpp $(CXXSOURCES) $(CXXHEADERS) $(SIMLIBS)
	SYNOPSYS_SIM_SETUP=$(TESTBENCH_PATH)/synopsys_sim.setup \
	vcs tb glbl -j$(NPROCS) $(WRAPPER_NAME) $< -Mdirectory=$@.tmp \
		$(VCS_CXXFLAGS) $(VCS_CXXDEFINES) $(VCS_INCLUDES) $(VCS_LDFLAGS) \
		$(VCS_VFLAGS) -o $@ -l $@.vcs.log

$(REGRESSION_TESTS): %: $(EXEC_PATH)/%
test_loader: %: $(EXEC_PATH)/%

# To include a test in cosimulation, the user defines a list of tests in
# REGRESSION_TESTS. The following two lines defines a rule named
# <test_name>.rule that is a dependency in <test_name>.log. These custom
# rules can be used to build RISC-V binaries for SPMD or CUDA tests.
USER_RULES=$(addsuffix .rule,$(REGRESSION_TESTS))
$(USER_RULES):

# Likewise - we define a custom rule for <test_name>.clean
USER_CLEAN_RULES=$(addsuffix .clean,$(REGRESSION_TESTS))
$(USER_CLEAN_RULES):

compilation.clean: 
	rm -rf $(EXEC_PATH)/DVEfiles
	rm -rf $(EXEC_PATH)/*.daidir $(EXEC_PATH)/*.tmp
	rm -rf $(EXEC_PATH)/64 $(EXEC_PATH)/.cxl*
	rm -rf $(EXEC_PATH)/*.vcs.log $(EXEC_PATH)/*.jou
	rm -rf $(EXEC_PATH)/*.key $(EXEC_PATH)/*.vpd
	rm -rf $(EXEC_PATH)/vc_hdrs.h
	rm -rf .vlogansetup* stack.info*
	rm -rf $(REGRESSION_TESTS) test_loader

.PHONY: help compilation.clean $(USER_RULES) $(USER_CLEAN_RULES)

