CL_DIR = $(CURDIR)
PROJECT = cl_manycore

# Image version is set by Bladerunner Makefile
FPGA_IMAGE_VERSION ?= 0.0.0 
CL_MANYCORE_RELEASE_VERSION  ?= $(shell echo $(FPGA_IMAGE_VERSION) | sed 's/\([0-9]*\)\.\([0-9]*\).\([0-9]*\)/000\10\20\3/')

include $(CL_DIR)/Makefile.environment

DEBUG ?= 
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?=

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build regression hardware

build:
	$(MAKE) -C $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) CL_MANYCORE_RELEASE_VERSION=$(CL_MANYCORE_RELEASE_VERSION)

regression:
	$(MAKE) -C regression $@ CL_DIR=$(CL_DIR) 

hardware:
	$(MAKE) -C hardware $@ CL_DIR=$(CL_DIR) 

cosim: 
	$(MAKE) -C testbenches $@ CL_DIR=$(CL_DIR) DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) AXI_PROT_CHECK=$(AXI_PROT_CHECK)

clean:
	$(MAKE) -C testbenches clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
	$(MAKE) -C build clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) 
	$(MAKE) -C regression clean CL_DIR=$(CL_DIR) 
	$(MAKE) -C hardware clean CL_DIR=$(CL_DIR) 

