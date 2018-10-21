# Foo

build:
ifndef PROJECT
	$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif
	CL_TOP_MODULE=$(PROJECT) ./aws_build_dcp_from_cl.sh -foreground

clean:
	rm -rf *.log *.jou *.out hd_visual src_post_encryption/ reports/ 
	rm -rf checkpoints last_log .Xil
