# Project specific variables
# Design name
TARGET           = cl_ipi
# Location of make-dependent IP (usually HLS IP that needs to be built)
IP_PATH          = ip
# List of make-dependent IPs in the IP_PATH directory
HLS_IPS          = 

# Board Specific Variables
BOARD            = F1
# FPGA Device
PART             = xcvu9p-flgb2104-2-i
# VERSION (For HLS) -- 7-Series: 1.0, Ultrascale: 2.0, Ultrascale+: 3.0
VERSION          = 3.0

# Computer specific variables
NJOBS            = 8

# AWS variables
SDK_DIR          = $(AWS_FPGA_REPO_DIR)/sdk
SDK_INC_DIR      = $(SDK_DIR)/userspace/include
SDK_UTILS_DIR    = $(SDK_DIR)/userspace/utils
HDK_SCRIPTS_DIR  = $(HDK_DIR)/common/shell_stable/new_cl_template/build/scripts/
HDK_SOFTWARE_DIR = $(HDK_COMMON_DIR)/software
AWS_DPI_SRC      = $(SDK_UTILS_DIR)/sh_dpi_tasks.c $(HDK_SOFTWARE_DIR)/src/fpga_pci_sv.c
AWS_DPI_INC      = $(SDK_INC_DIR) $(SDK_UTILS_DIR) $(HDK_SOFTWARE_DIR)/include $(HDK_SOFTWARE_DIR)/src

# CL Variables
CL_DIR           = $(PWD)
CL_SCRIPTS_DIR   = $(CL_DIR)/scripts

# Cosimulation Variabls
COSIM_INC_DIR    = $(CL_DIR)/software/include
COSIM_SRC_DIR    = $(CL_DIR)/software
# COSIM_SRC names the software C files that drive cosimulation
COSIM_SRC        = $(COSIM_SRC_DIR)/$(TARGET).c
VIVADO_SIM_PATH  = $(TARGET)/$(TARGET).sim/sim_1/behav/xsim/

# Finally, build up the compiler options
INCLUDES         = $(AWS_DPI_INC) $(COSIM_INC_DIR)
CFLAGS           = -DVIVADO_SIM -DSV_TEST

# Auto-generated (do not modify)
# CL_DIR is used by the AWS F1 flow
HLS_TARGETS      = $(foreach IP,$(HLS_IPS), $(IP_PATH)/$(IP)/build/$(PART).$(IP))
XSC_INCLUDES     = $(foreach dir,$(INCLUDES),--additional_option -I$(dir))
XSC_FLAGS        = $(foreach flag,$(CFLAGS),--additional_option $(flag))

.PHONY: clean extraclean
.PRECIOUS:%.log

# Setup creates the vivado project, builds the block diagram in cl.tcl, and adds HDL files
setup: $(TARGET) 
$(TARGET): $(TARGET).setup.log
$(TARGET).setup.log: $(HLS_TARGETS) cl.tcl
	vivado -mode tcl -source $(CL_SCRIPTS_DIR)/setup.tcl -log $@ -tclargs $(TARGET) $(CL_DIR)

# HLS_TARGETS builds each of the HLS ips specified in HLS_IPS
$(HLS_TARGETS):
	PART=$(PART) VERSION=$(VERSION) $(MAKE) -C $(subst build/,,$(dir $@))

# Compile builds the final .dcp file
compile: $(TARGET).compile.log
$(TARGET).compile.log: $(TARGET)
	vivado $(TARGET)/$(TARGET).xpr -mode tcl -source $(CL_SCRIPTS_DIR)/compile.tcl -log $@ -tclargs $(NJOBS)

# rtlsim runs RTL simulation
rtlsim: $(TARGET) $(TARGET).rtlsim.log
$(TARGET).rtlsim.log: $(TARGET) 
	rm -f $(TARGET).cosim.setup.log
	vivado $(TARGET)/$(TARGET).xpr -mode tcl -source $(CL_SCRIPTS_DIR)/rtlsim.tcl \
		 -log $@ -tclargs cl_top test_cl

# The following are cosimulation variables.
# The cosimulation flow is tricky. The tl;dr flow is:
# 1. Open the Vivado project, and re-define the CL_NAME and TEST_NAME variables
#    that switch between testbenches (cosim.setup.log)
# 2. Generate the simulation scripts, pointing at cosimulation files
#    (cosim.setup.log)
# 3. Compile the simulation using the generated script (compile.log)
# 4. Compile the C Cosimulation files (User and AWS files) into a dpi.so file
#    and replace the dpi.so file for rtlsimulation
# 5. Elaborate the design using the generated script. This points at all the
#    correct files and IP versions in the block diagram
# 6. Run cosimulation
$(TARGET).cosim.setup.log: $(TARGET)
	rm -f $(TARGET).rtlsim.log
	vivado $(TARGET)/$(TARGET).xpr -mode tcl -source $(CL_SCRIPTS_DIR)/cosim.tcl \
		-log $@ -tclargs cl_top test_cosim 

$(VIVADO_SIM_PATH)/compile.log: $(TARGET).cosim.setup.log
	cd $(VIVADO_SIM_PATH); ./compile.sh

$(VIVADO_SIM_PATH)/dpi.so: $(TARGET).cosim.setup.log $(VIVADO_SIM_PATH)/compile.log $(COSIM_SRC) $(AWS_DPI_SRC)
	xsc $(COSIM_SRC) $(AWS_DPI_SRC) $(XSC_INCLUDES) $(XSC_FLAGS) -o $@

$(VIVADO_SIM_PATH)/elaborate.log: $(VIVADO_SIM_PATH)/dpi.so
	cd $(VIVADO_SIM_PATH); ./elaborate.sh

cosim: $(TARGET).cosim.log 
$(TARGET).cosim.log: $(VIVADO_SIM_PATH)/elaborate.log
	cd $(VIVADO_SIM_PATH); xsim tb_behav -tclbatch $(CL_DIR)/testbenches/cosim/waves.tcl -log $(CL_DIR)/$@ 

# Finally, clean-up files
clean: 
	rm -f *.log *.jou *.tmp $(VIVADO_SIM_PATH)/dpi.so $(VIVADO_SIM_PATH)/*.log *.pb 
	rm -rf xsim.dir

extraclean: clean 
	rm -rf $(TARGET) $(TARGET).touch 
	$(foreach IP,$(HLS_IPS), PART=$(PART) VERSION=$(VERSION) make -C $(IP_PATH)/$(IP) clean;)
