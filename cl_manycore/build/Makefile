include ../Makefile.environment

HARDWARE_PATH=../hardware

.PHONY:$(HARDWARE_PATH)

$(HARDWARE_PATH): envcheck
	$(MAKE) -C $@

build: envcheck checkpoints/to_aws/$(PROJECT).Developer_CL.tar

checkpoints/to_aws/$(PROJECT).Developer_CL.tar: envcheck $(HARDWARE_PATH) 
	CL_TOP_MODULE=$(PROJECT) CL_DIR=$(CL_DIR) ./aws_build_dcp_from_cl.sh \
		-foreground -design $(PROJECT)

clean:
	rm -rf *.log *.jou *.out hd_visual *.tmp 
	rm -rf checkpoints last_log .Xil .cache src_post_encryption/ reports/
