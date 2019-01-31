ifndef HDK_DIR
$(error "HDK variables undefined. Run `source hdk_setup.sh` in aws-fpga")
endif

CL_DIR ?= $(PWD)
PROJECT = cl_app
DEBUG ?= 

SIM_TARGETS=rtlsim cosim
SW_TARGETS=software
BUILD_TARGETS=build

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build

$(BUILD_TARGETS):
	make -C $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) BSG_IP_CORES_DIR=$(BSG_IP_CORES_DIR)

$(SW_TARGETS):
	make -C $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)

$(SIM_TARGETS):
	make -C testbenches $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) DEBUG=$(DEBUG)

clean:
	make -C testbenches clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
	make -C build clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
	make -C software clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
