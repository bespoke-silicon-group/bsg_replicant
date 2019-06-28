include ../Makefile.environment
include $(CL_DIR)/Makefile.hdk
include $(HARDWARE_PATH)/Makefile.include

.DEFAULT_GOAL=build
.PHONY: clean build

build: checkpoints/to_aws/$(PROJECT).Developer_CL.tar

checkpoints/to_aws/$(PROJECT).Developer_CL.tar: $(HARDWARE_TARGETS)
	CL_TOP_MODULE=$(PROJECT) CL_DIR=$(CL_DIR) ./aws_build_dcp_from_cl.sh \
		-foreground -design $(PROJECT)

clean: hardware.clean
	rm -rf *.log *.jou *.out hd_visual *.tmp 
	rm -rf checkpoints last_log .Xil .cache src_post_encryption/ reports/

