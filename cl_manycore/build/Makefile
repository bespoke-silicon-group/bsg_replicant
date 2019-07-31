include ../Makefile.environment
include $(CL_DIR)/Makefile.hdk
include $(HARDWARE_PATH)/Makefile.include

# The following makefile fragment verifies that the tools and CAD environment is
# configured correctly. Makefile.environment must be included before this line
include $(TESTBENCH_PATH)/Makefile.cadenv

.DEFAULT_GOAL=build
.PHONY: clean build

build: checkpoints/to_aws/$(PROJECT).Developer_CL.tar
checkpoints/to_aws/$(PROJECT).Developer_CL.tar: $(HARDWARE_TARGETS)
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	BSG_IP_CORES_DIR=$(BASEJUMP_STL_DIR) \
	XILINX_IP=$(XILINX_IP) \
	HDK_COMMON_DIR=$(HDK_COMMON_DIR) \
	HDK_SHELL_DESIGN_DIR=$(HDK_SHELL_DESIGN_DIR) \
	HDK_SHELL_DIR=$(HDK_SHELL_DIR) \
	HDK_DIR=$(HDK_DIR) \
	XILINX_VIVADO=$(XILINX_VIVADO) \
	CL_TOP_MODULE=$(PROJECT) CL_DIR=$(CL_DIR) ./aws_build_dcp_from_cl.sh \
		-foreground -design $(PROJECT)

clean: hardware.clean
	rm -rf *.log *.jou *.out hd_visual *.tmp 
	rm -rf checkpoints last_log .Xil .cache src_post_encryption/ reports/

