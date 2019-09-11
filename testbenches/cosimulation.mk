# This Makefile Fragment enables cosimulation
#
# environment.mk verifies the build environment and sets the following
# variables
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../../environment.mk 

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

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly. environment.mk must be included before this line
include $(CL_DIR)/cadenv.mk

# The following variables are set by $(CL_DIR)/hdk.mk
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(CL_DIR)/hdk.mk

# $(HARDWARE_PATH)/hardware.mk adds to VSOURCES which is a list of verilog
# source files for cosimulation and compilation, and VHEADERS, which is similar,
# but for header files. It also adds to CLEANS, a list of clean rules for
# cleaning hardware targets.
include $(HARDWARE_PATH)/hardware.mk

# simlibs.mk defines build rules for hardware and software simulation libraries
# that are necessary for running cosimulation. These are dependencies for
# regression since running $(MAKE) recursively does not prevent parallel builds
# of identical rules -- which causes errors.
#
# simlibs.mk adds to SIMLIBS and CLEANS variables
include $(TESTBENCH_PATH)/simlibs.mk

# -------------------- Arguments --------------------
# This Makefile has several optional "arguments" that are passed as Variables
#
# AXI_MEMORY_MODEL: Use an SRAM-like Memory model that increases simulation
#                   speed. Default: 1
# AXI_PROT_CHECK: Enables PCIe Protocol checker. Default: 0
# DEBUG: Opens the GUI during cosimulation. Default: 0
# TURBO: Disables VPD generation. Default: 0
# EXTRA_TURBO: Disables VPD Generation, and more optimization flags: Default 0
# 
# If you need additional speed, you can set EXTRA_TURBO=1 during compilation. 
# This is a COMPILATION ONLY option. Any subsequent runs, without compilation
# will retain this setting
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK   ?= 0
DEBUG            ?= 0
TURBO            ?= 0
EXTRA_TURBO      ?= 0

# -------------------- VARIABLES --------------------
# We parallelize VCS compilation, but we leave a few cores on the table.
NPROCS = $(shell echo "(`nproc`/4 + 1)" | bc)

# Name of the cosimulation wrapper system verilog file.
WRAPPER_NAME = cosim_wrapper

# libfpga_mgmt will be compiled in $(TESTBENCH_PATH)
LDFLAGS    += -L$(TESTBENCH_PATH) -Wl,-rpath=$(TESTBENCH_PATH)

# libbsg_manycore_runtime will be compiled in $(LIBRARIES_PATH)
LDFLAGS    += -L$(LIBRARIES_PATH) -Wl,-rpath=$(LIBRARIES_PATH)
# The bsg_manycore_runtime headers are in $(LIBRARIES_PATH) (for cosimulation)
INCLUDES   += -I$(LIBRARIES_PATH) 

CDEFINES   += -DCOSIM -DVCS
CXXDEFINES += -DCOSIM -DVCS
CXXFLAGS   += -lstdc++


# So that we can limit tool-specific to a few specific spots we use VDEFINES,
# VINCLUDES, and VSOURCES to hold lists of macros, include directores, and
# verilog headers, and sources (respectively). These are used during simulation
# compilation, but transformed into a tool-specific syntax where necesssary.
VDEFINES   += VCS_SIM
VDEFINES   += DISABLE_VJTAG_DEBUG

ifeq ($(AXI_PROT_CHECK),1)
VDEFINES   += ENABLE_PROTOCOL_CHK
endif

ifeq ($(AXI_MEMORY_MODEL),1)
VDEFINES   += AXI_MEMORY_MODEL=1
VDEFINES   += ECC_DIRECT_EN
VDEFINES   += RND_ECC_EN
VDEFINES   += ECC_ADDR_LO=0
VDEFINES   += ECC_ADDR_HI=0
VDEFINES   += RND_ECC_WEIGHT=0
endif

# The manycore architecture unsynthesizable simulation sources (for tracing,
# etc) are defined in sim_filelist.mk. It adds to VSOURCES, VHEADERS, and
# VINCLUDES and uses the variable BSG_MANYCORE_DIR
include $(BSG_MANYCORE_DIR)/machines/sim_filelist.mk

# Custom Logic (CL) source directories
VINCLUDES += $(TESTBENCH_PATH)

VSOURCES += $(TESTBENCH_PATH)/$(WRAPPER_NAME).sv

# Using the generic variables VSOURCES, VINCLUDES, and VDEFINES, we create
# tool-specific versions of the same variables. VHEADERS must be compiled before
# VSOURCES.
VLOGAN_SOURCES  += $(VHEADERS) $(VSOURCES) 
VLOGAN_INCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VLOGAN_DEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VLOGAN_FILELIST += $(TESTBENCH_PATH)/aws.vcs.f
VLOGAN_VFLAGS   += -ntb_opts tb_timescale=1ps/1ps -timescale=1ps/1ps \
                   -sverilog +systemverilogext+.svh +systemverilogext+.sv \
                   +libext+.sv +libext+.v +libext+.vh +libext+.svh \
                   -full64 -lca -v2005 +v2k +lint=TFIPC-L -assert svaext

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
VLOGAN_VFLAGS += -undef_vcs_macro
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

# -------------------- TARGETS --------------------
.PRECIOUS: $(SIM_PATH)/%.log $(SIM_PATH)/%

$(SIM_PATH)/synopsys_sim.setup: $(TESTBENCH_PATH)/synopsys_sim.setup
	cp $< $@

$(SIM_PATH)/compile.vlogan.log: $(SIM_PATH)/synopsys_sim.setup
$(SIM_PATH)/compile.vlogan.log: $(CL_DIR)/Makefile.machine.include
$(SIM_PATH)/compile.vlogan.log: $(VHEADERS) $(VSOURCES)
	XILINX_IP=$(XILINX_IP) \
	HDK_COMMON_DIR=$(HDK_COMMON_DIR) \
	HDK_SHELL_DESIGN_DIR=$(HDK_SHELL_DESIGN_DIR) \
	HDK_SHELL_DIR=$(HDK_SHELL_DIR) \
	XILINX_VIVADO=$(XILINX_VIVADO) \
	vlogan $(VLOGAN_VFLAGS) $(VLOGAN_DEFINES) $(VLOGAN_SOURCES) \
		-f $(TESTBENCH_PATH)/aws.vcs.f $(VLOGAN_DEFINES)  \
		$(VLOGAN_INCLUDES) -l $@.prelim
	mv $@.prelim $@

# VCS Generates an executable file by compiling the $(SRC_PATH)/%.c or
# $(SRC_PATH)/%.cpp file that corresponds to the target test in the
# $(SRC_PATH) directory. To allow users to attach test-specific makefile
# rules, each test has a corresponding <test_name>.rule that can have additional
# dependencies
$(SIM_PATH)/%: $(SRC_PATH)/%.c $(SIM_PATH)/compile.vlogan.log $(SIMLIBS)
	vcs tb glbl -j$(NPROCS) $(WRAPPER_NAME) $< -Mdirectory=$@.tmp \
		$(VCS_CFLAGS) $(VCS_CDEFINES) $(VCS_INCLUDES) $(VCS_LDFLAGS) \
		$(VCS_VFLAGS) -o $@ -l $@.vcs.log

$(SIM_PATH)/%: $(SRC_PATH)/%.cpp $(SIM_PATH)/compile.vlogan.log $(SIMLIBS)
	vcs tb glbl -j$(NPROCS) $(WRAPPER_NAME) $< -Mdirectory=$@.tmp \
		$(VCS_CXXFLAGS) $(VCS_CXXDEFINES) $(VCS_INCLUDES) $(VCS_LDFLAGS) \
		$(VCS_VFLAGS) -o $@ -l compile.vcs.log

$(REGRESSION_TESTS): %: $(SIM_PATH)/%
test_unified_main: %: $(SIM_PATH)/%

# To include a test in cosimulation, the user defines a list of tests in
# COSIMULATION_REGRESSION_TESTS. The following two lines defines a rule named
# <test_name>.rule that is a dependency in <test_name>.log. These custom
# rules can be used to build RISC-V binaries for SPMD or CUDA tests.
USER_RULES=$(addsuffix .rule,$(REGRESSION_TESTS))
$(USER_RULES):

# Likewise - we define a custom rule for <test_name>.clean
USER_CLEAN_RULES=$(addsuffix .clean,$(REGRESSION_TESTS))
$(USER_CLEAN_RULES):

# The following rules generate the <test_name>.log file by running the
# <test_name> executable. The output .vpd file will be named <test_name>.vpd.
#
# The two following rules appear similar but they are not! The first rule
# matches with all tests in INDEPENDENT_TESTS, and the second matches with
# all rules in UNIFIED_TESTS
INDEPENDENT_LOGS=$(foreach tgt, $(INDEPENDENT_TESTS), $(SIM_PATH)/$(tgt).log)
$(INDEPENDENT_LOGS): $(SIM_PATH)/%.log: $(SIM_PATH)/% %.rule
	$< 2>&1 +ntb_random_seed_automatic +vpdfile+$<.vpd $(SIM_ARGS)\
		+c_args="$(C_ARGS)" | tee $@

$(SIM_PATH)/%.log: $(SIM_PATH)/test_unified_main %.rule
	$< 2>&1 +ntb_random_seed_automatic +vpdfile+$<.vpd $(SIM_ARGS) \
		+c_args="$(subst .log,,$(notdir $@))" | tee $@

# We build a list of LOG_RULES for the regression rule (below)
LOG_RULES = $(addsuffix .log,$(REGRESSION_TESTS))
$(LOG_RULES): %: $(SIM_PATH)/%

regression: $(SIM_PATH)/regression.log 

$(SIM_PATH)/regression.log: $(LOG_RULES) 
	@pass=0; total=0; \
	echo ""| tee $@; \
	echo ""| tee -a $@; \
	echo "Parsing $(REGRESSION_TESTS_TYPE) Regression Test results..."| tee -a $@; \
	echo ""| tee -a $@; \
	echo ""| tee -a $@; \
	for target in $(basename $(basename $?)); do \
		if grep "BSG COSIM FAIL" $$target.log > /dev/null; then \
			echo "FAIL: Regression Test $$target failed!"| tee -a $@; \
		else \
			echo "PASS: Regression Test $$target passed!"| tee -a $@; \
			let "pass+=1"; \
		fi; \
		let "total+=1"; \
	done; \
	if [ ! $$pass == $$total ]; then \
		echo "==================================================="| tee -a $@; \
		echo "" | tee -a $@; \
		echo "FAIL! $$pass out of $$total $(REGRESSION_TESTS_TYPE) regression tests passed"| tee -a $@; \
		echo "" | tee -a $@; \
		echo "==================================================="| tee -a $@; \
		exit 1| tee -a $@; \
	fi; \
	echo "==========================================================="| tee -a $@; \
	echo ""| tee -a $@; \
	echo "PASS! All $$total tests passed for $(REGRESSION_TESTS_TYPE)"| tee -a $@; \
	echo ""| tee -a $@; \
	echo "==========================================================="| tee -a $@;

clean: $(USER_CLEAN_RULES)
	rm -rf $(SIM_PATH)/AN.DB $(SIM_PATH)/DVEfiles
	rm -rf $(SIM_PATH)/*.daidir $(SIM_PATH)/*.tmp
	rm -rf $(SIM_PATH)/64 $(SIM_PATH)/.cxl*
	rm -rf $(SIM_PATH)/*.log $(SIM_PATH)/*.jou
	rm -rf $(SIM_PATH)/synopsys_sim.setup
	rm -rf $(SIM_PATH)/*.key $(SIM_PATH)/*.vpd
	rm -rf $(SIM_PATH)/vc_hdrs.h
	rm -rf .vlogansetup* stack.info*
	rm -rf $(REGRESSION_TESTS) test_unified_main

.PHONY: help regression clean $(USER_RULES) $(USER_CLEAN_RULES)

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<test_name>|<test_name>.log}"
	@echo "      regression: Run all cosimulation regression tests for "
	@echo "             this subdirectory"
	@echo "      <test_name>: Build the cosimulation binary for a specific"
	@echo "             test"
	@echo "      <test_name.log>: Run a specific cosimulation test and "
	@echo "             generate the log file"
	@echo "      clean: Remove all subdirectory-specific outputs"
