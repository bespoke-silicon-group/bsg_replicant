TARGET=cl_ipi
NJOBS=8
CL_DIR = $(PWD)

CL_SCRIPTS_DIR=$(CL_DIR)/scripts
HDK_SCRIPTS_DIR=$(HDK_DIR)/common/shell_stable/new_cl_template/build/scripts/

BUILD_SCRIPTS=$(CL_SCRIPTS_DIR)/aws_build_dcp_from_cl.sh $(CL_SCRIPTS_DIR)/create_dcp_from_cl.tcl

.PHONY: clean extraclean

setup: $(TARGET)
$(TARGET): cl.tcl
	vivado -mode tcl -source $(CL_SCRIPTS_DIR)/setup.tcl -tclargs $@ $(CL_DIR)

compile: $(TARGET).touch
$(TARGET).touch: $(TARGET)
	vivado $(TARGET)/$(TARGET).xpr -mode tcl -source $(CL_SCRIPTS_DIR)/compile.tcl -tclargs $(NJOBS)
	touch $@


simulate: $(TARGET)  $(TARGET).simulate.log
$(TARGET).simulate.log: $(TARGET) 
	vivado $(TARGET)/$(TARGET).xpr -mode tcl -source $(CL_SCRIPTS_DIR)/simulate.tcl -tclargs $(NJOBS) | tee $(TARGET).simulate.log

clean:
	rm -f *.log *.jou *.tmp

extraclean: clean
	rm -rf $(TARGET) $(TARGET).touch
