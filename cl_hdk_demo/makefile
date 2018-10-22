ifndef HDK_DIR
$(error "HDK variables undefined. Run `source hdk_setup.sh` in aws-fpga")
endif

export CL_DIR ?= $(PWD)
PROJECT = cl_demo
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
