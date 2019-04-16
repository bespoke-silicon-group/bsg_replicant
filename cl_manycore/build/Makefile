include ../Makefile.environment
include $(HARDWARE_PATH)/Makefile.hardware

.PHONY: clean build

build: checkpoints/to_aws/$(PROJECT).Developer_CL.tar

checkpoints/to_aws/$(PROJECT).Developer_CL.tar: envcheck $(HARDWARE_TARGETS)
	CL_TOP_MODULE=$(PROJECT) CL_DIR=$(CL_DIR) ./aws_build_dcp_from_cl.sh \
		-foreground -design $(PROJECT)

clean: clean.hardware
	rm -rf *.log *.jou *.out hd_visual *.tmp 
	rm -rf checkpoints last_log .Xil .cache src_post_encryption/ reports/
