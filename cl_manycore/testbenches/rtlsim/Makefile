include ../../Makefile.environment
ifndef PROJECT
$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif

# AWS Paths (Don't modify)
export XILINX_IP    = $(HDK_SHELL_DESIGN_DIR)/ip
SDK_DIR             = $(AWS_FPGA_REPO_DIR)/sdk
C_COMMON_DIR        = $(HDK_COMMON_DIR)/software
C_SDK_USR_INC_DIR   = $(SDK_DIR)/userspace/include
C_SDK_USR_UTILS_DIR = $(SDK_DIR)/userspace/utils

# Simulation Variables (Modify if directory structure changes)
RTL_TB_MODULE = cl_tb
SCRIPTS_DIR   = $(CL_DIR)/testbenches/rtlsim
# SIM_ROOT used by included makefiles
SIM_ROOT      = $(CL_DIR)/testbenches/sim/rtlsim

# Cosim C-File Variables (Modify C_TB_TOP if the top file is not $(PROJECT).c
C_INC_DIR = $(CL_DIR)/software/include
C_SRC_DIR = $(CL_DIR)/software
# C_TB_TOP names the software C file that drives cosimulation
C_TB_TOP  = $(HDK_COMMON_DIR)/software/src/test_null.c
C_FILES   = $(C_TB_TOP) $(C_SDK_USR_UTILS_DIR)/sh_dpi_tasks.c \
		$(C_COMMON_DIR)/src/fpga_pci_sv.c 

# SV TEST LIST used by included makefiles (Purpose unclear)
SV_TEST_LIST = $(PROJECT)

ifdef CHECK
XVLOG_OPTS +=-d ENABLE_PROTOCOL_CHK
XELOG_OPTS +=$(XVLOG_OPTS)
endif

ifdef DEBUG
XSIM_OPTS +=-gui
export GUI
endif
ifdef AXI_MEMORY_MODEL
XVLOG_OPTS +=-d AXI_MEMORY_MODEL=1 -d ECC_DIRECT_EN -d RND_ECC_EN -d ECC_ADDR_LO=0 -d ECC_ADDR_HI=0 -d RND_ECC_WEIGHT=0
endif
run: make_sim_dir compile sim

include $(HDK_COMMON_DIR)/verif/tb/scripts/Makefile.common.inc
