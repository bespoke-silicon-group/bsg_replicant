ifndef HDK_DIR
$(error "HDK variables undefined. Run `source hdk_setup.sh` in aws-fpga")
endif

# ifndef BSG_IP_DIR
# $(info "BSG_IP_DIR set to default")
# export BSG_IP_DIR = $(PWD)/../../bsg_ip_cores
# endif

# ifndef BSG_DESIGNS_DIR
# $(info "BSG_DESIGNS_DIR set to default")
# export BSG_DESIGNS_DIR = $(PWD)/../../bsg_designs
# endif

export BSG_IP_DIR ?= $(PWD)/../../bsg_ip_cores
export BSG_DESIGNS_DIR ?= $(PWD)/../../bsg_designs

export CL_DIR ?= $(PWD)
PROJECT = cl_fsb
GUI ?= 

SIM_TARGETS=rtlsim cosim
SW_TARGETS=software
BUILD_TARGETS=build

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build

$(BUILD_TARGETS):
	PROJECT=$(PROJECT) make -C $@

$(SW_TARGETS):
	PROJECT=$(PROJECT) make -C $@

$(SIM_TARGETS):
	PROJECT=$(PROJECT) GUI=$(GUI) make -C testbenches $@

clean:
	make -C testbenches clean
	make -C build clean
	make -C software clean
	rm -f *.log *.jou *.pb
