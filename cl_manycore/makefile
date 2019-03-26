ifndef HDK_DIR
$(error "HDK variables undefined. Run `source hdk_setup.sh` in aws-fpga")
endif

CL_DIR ?= $(CURDIR)
PROJECT = cl_manycore
DEBUG ?= 
AXI_MEMORY_MODEL ?=

SIM_TARGETS=rtlsim cosim
SW_TARGETS=software
BUILD_TARGETS=build

BSG_IP_CORES_DIR ?= $(CL_DIR)/../../bsg_ip_cores/
BSG_MANYCORE_DIR ?= $(CL_DIR)/../../bsg_manycore

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build configuration_rom

build: configuration_rom
	make -C $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) BSG_IP_CORES_DIR=$(BSG_IP_CORES_DIR) BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR)

$(SW_TARGETS):
	make -C $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)

$(SIM_TARGETS): configuration_rom
	make -C testbenches $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL)

configuration_rom:
	rm ../hdl/$@.v
	python $(BSG_IP_CORES_DIR)/bsg_mem/bsg_ascii_to_rom.py ../hdl/bsg_bladerunner.rom $@ >> ../hdl/$@.v

clean:
	make -C testbenches clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
	make -C build clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) BSG_IP_CORES_DIR=$(BSG_IP_CORES_DIR) BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR)
	make -C software clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) DEBUG=$(DEBUG)
