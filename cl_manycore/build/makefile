.PHONY:check

check:
ifndef PROJECT
	$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif
ifndef CL_DIR
	$(error "CL_DIR undefined. Set PROJECT before running this makefile")
endif

build: check
	CL_TOP_MODULE=$(PROJECT) CL_DIR=$(CL_DIR) ./aws_build_dcp_from_cl.sh -foreground 

clean:
	rm -rf *.log *.jou *.out hd_visual src_post_encryption/ reports/ 
	rm -rf checkpoints last_log .Xil .cache
