ifndef HDK_DIR
$(error "HDK variables undefined. Run `source hdk_setup.sh` in aws-fpga")
endif

ifndef BSG_IP_DIR
$(info "BSG_IP_DIR set to default")
export BSG_IP_DIR = $(PWD)/../../bsg_ip_cores
endif

ifndef BSG_DESGINS_DIR
$(info "BSG_DESGINS_DIR set to default")
export BSG_DESGINS_DIR = $(PWD)/../../bsg_designs
endif

export CL_DIR ?= $(PWD)
PROJECT = cl_fsb
GUI ?= 

SIM_TARGETS=rtlsim cosim
SW_TARGETS=software
BUILD_TARGETS=build

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build ip

ip:
	PROJECT=$(PROJECT) make -C $@ 

$(BUILD_TARGETS): ip
	PROJECT=$(PROJECT) make -C $@

$(SW_TARGETS): ip
	PROJECT=$(PROJECT) make -C $@

$(SIM_TARGETS): ip
	PROJECT=$(PROJECT) GUI=$(GUI) make -C testbenches $@

clean:
	make -C testbenches clean
	make -C build clean
	make -C software clean
	make -C ip clean
	rm -f *.log *.jou *.pb
