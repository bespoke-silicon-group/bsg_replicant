include environment.mk

PROJECT = cl_manycore

DEBUG ?= 
AXI_MEMORY_MODEL ?= 1
AXI_PROT_CHECK ?=
TURBO ?= 1 # Use if you don't want debug information
EXTRA_TURBO ?= 0 # Use at your own risk.

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build regression hardware

build:
	$(MAKE) -C $@ PROJECT=$(PROJECT) CL_MANYCORE_RELEASE_VERSION=$(CL_MANYCORE_RELEASE_VERSION)

regression:
	$(MAKE) -C regression $@ 

hardware:
	$(MAKE) -C hardware $@ 

cosim: 
	$(MAKE) -C testbenches regression DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL) \
		AXI_PROT_CHECK=$(AXI_PROT_CHECK) TURBO=$(TURBO) EXTRA_TURBO=$(EXTRA_TURBO)

clean:
	$(MAKE) -C testbenches clean 
	$(MAKE) -C build clean 
	$(MAKE) -C regression clean
	$(MAKE) -C hardware clean 

