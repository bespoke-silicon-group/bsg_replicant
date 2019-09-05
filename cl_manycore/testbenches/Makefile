# This Makefile is for running cosimulation regression
#
# Makefile.environment verifies the build environment and sets some of the
# variables described above. The following variables are typically set by
# Makefile.environment.
#
# TESTBENCH_PATH: The path to the testbench directory in the bsg_f1 repository
# LIBRAIRES_PATH: The path to the libraries directory in the bsg_f1 repository
# HARDARE_PATH: The path to the hardware directory in the bsg_f1 repository
# BASEJUMP_STL_DIR: Path to a clone of BaseJump STL
# BSG_MANYCORE_DIR: Path to a clone of BSG Manycore
# CL_DIR: Path to the directory of this AWS F1 Project
include ../environment.mk

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly. Makefile.environment must be included before this line
include $(CL_DIR)/cadenv.mk

# $(HARDWARE_PATH)/hardware.mk adds to VSOURCES which is a list of verilog
# source files for cosimulation and compilation, and VHEADERS, which is similar,
# but for header files. It also adds to CLEANS, a list of clean rules for
# cleaning hardware targets.
include $(HARDWARE_PATH)/hardware.mk

# simlibs.mk defines build rules for hardware and software libraries
# that are necessary for running cosimulation. These are dependencies for
# regression since running $(MAKE) recursively does not prevent parallel builds
# of identical rules -- which causes errors.
#
# simlibs.mk adds to LIBRARIES and CLEANS variables
include simlibs.mk

# The F1 Co-simulation sources are defined in cosimulation.mk. It adds to
# VSOURCES, VHEADERS, and VINCLUDES and uses the variable BSG_MANYCORE_DIR
include $(TESTBENCH_PATH)/cosimulation.mk

# -------------------- Arguments --------------------
#
# This Makefile has several optional "arguments" that are passed as Variables
#
# AXI_MEMORY_MODEL: Use an SRAM-like Memory model that increases simulation
#                   speed. Default: 1
# AXI_PROT_CHECK: Enables PCIe Protocol checker. Default: 0
# DEBUG: Opens the GUI during cosimulation. Default: 0
# TURBO: Disables VPD generation. Default: 1
# EXTRA_TURBO: Disables VPD Generation, and more optimization flags: Default 0
# 
# If you need additional speed, you can set EXTRA_TURBO=1 during compilation. 
# This is a COMPILATION ONLY option. Any subsequent runs, without compilation
# will retain this setting

DEBUG ?= 0
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?= 0
TURBO ?= 1 # Use if you don't want debug information
EXTRA_TURBO ?= 0 # Use at your own risk.

# -------------------- TARGETS --------------------
#
# The TARGETS are the subdirectories containing regression tests
TARGETS = library spmd cuda python
CLEANS += $(addsuffix .clean,$(TARGETS))
.PHONY: clean %.clean regression $(TARGETS)

all: regression

regression: $(TARGETS)
$(TARGETS): $(VHEADERS) $(VSOURCES) $(LIBRARIES)
	$(MAKE) -C  $@ regression EXTRA_TURBO=$(EXTRA_TURBO) 			\
		DEBUG=$(DEBUG) AXI_PROT_CHECK=$(AXI_PROT_CHECK) 		\
		TURBO=$(TURBO) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL)

clean: $(CLEANS)
	rm -rf *.log *.jou
	rm -rf .cxl*
	rm -rf *.bak

%.clean:
	$(MAKE) -C $(basename $@) clean
	rm -rf .Xil

