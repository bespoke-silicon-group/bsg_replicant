PROJECT = cl_manycore

# Image version is set by Bladerunner Makefile
FPGA_IMAGE_VERSION ?= 0.0.0 
CL_MANYCORE_RELEASE_VERSION  ?= $(shell echo $(FPGA_IMAGE_VERSION) | sed 's/\([0-9]*\)\.\([0-9]*\).\([0-9]*\)/000\10\20\3/')

DEBUG ?= 
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?=
TURBO ?= 1

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build regression hardware

build:
	$(MAKE) -C $@ PROJECT=$(PROJECT) CL_MANYCORE_RELEASE_VERSION=$(CL_MANYCORE_RELEASE_VERSION)

regression:
	$(MAKE) -C regression $@ 

hardware:
	$(MAKE) -C hardware $@ 

cosim: 
	$(MAKE) -C testbenches $@ DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) AXI_PROT_CHECK=$(AXI_PROT_CHECK) TURBO=$(TURBO)

clean:
	$(MAKE) -C testbenches clean 
	$(MAKE) -C build clean 
	$(MAKE) -C regression clean
	$(MAKE) -C hardware clean 

