CL_DIR = $(CURDIR)
PROJECT = cl_manycore

include $(CL_DIR)/Makefile.environment

DEBUG ?= 
AXI_MEMORY_MODEL ?=

all: $(SIM_TARGETS) $(BUILD_TARGETS)

.PHONY: build regression hardware

build: 
	$(MAKE) -C $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)

regression:
	$(MAKE) -C regression $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)

hardware:
	$(MAKE) -C hardware $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)

cosim: 
	$(MAKE) -C testbenches $@ CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) DEBUG=$(DEBUG) AXI_MEMORY_MODEL=$(AXI_MEMORY_MODEL)

clean:
	$(MAKE) -C testbenches clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
	$(MAKE) -C build clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT) 
	$(MAKE) -C regression clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
	$(MAKE) -C hardware clean CL_DIR=$(CL_DIR) PROJECT=$(PROJECT)
