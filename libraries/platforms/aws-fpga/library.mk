# Copyright (c) 2019, University of Washington All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
# 
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
# 
# Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# aws-fpga and aws-vcs are identical, EXCEPT for the MMIO layer. The
# reuse the bsg_manycore_platform.cpp file between the two platforms
# aws-fpga, but provide our own bsg_manycore_mmio.cpp file that
# handles PCIE-based MMIO.
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/bsg_manycore_mmio.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/bsg_manycore_platform.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/features/profiler/simulation/bsg_manycore_profiler.cpp

# aws-fpga does not provide a DMA feature. Therefore, we use the fragment in 
# features/dma/noimpl/feature.mk that simply returns
# HB_MC_NO_IMPL for each function call.
include $(LIBRARIES_PATH)/features/dma/noimpl/feature.mk

PLATFORM_OBJECTS += $(patsubst %cpp,%o,$(PLATFORM_CXXSOURCES))
PLATFORM_OBJECTS += $(patsubst %c,%o,$(PLATFORM_CSOURCES))

# -DCOSIM is still necessary, for now
$(PLATFORM_OBJECTS): INCLUDES := -I$(LIBRARIES_PATH)
$(PLATFORM_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/platforms/aws-fpga
$(PLATFORM_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/dma
$(PLATFORM_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/profiler
# not sure if these are still necessary for AWS, if fpga_mgmt is installed
$(PLATFORM_OBJECTS): INCLUDES += -I$(SDK_DIR)/userspace/include
$(PLATFORM_OBJECTS): CFLAGS   := -std=c11 -fPIC -D_GNU_SOURCE $(INCLUDES)
$(PLATFORM_OBJECTS): CXXFLAGS := -std=c++11 -fPIC -D_GNU_SOURCE $(INCLUDES)

$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: $(PLATFORM_OBJECTS)

# libfpga_mgmt is the platform library provided by AWS.
$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS += -lfpga_mgmt

_DOCSTRING := "Rules from aws-fpga/library.mk\n"
_TARGETS :=

_TARGETS +="install"
_DOCSTRING += "    install:\n"
_DOCSTRING += "        - Install libbsg_manycore_runtime.so in\n"
_DOCSTRING += "          /usr/lib64 and the headers in /usr/include\n"
install: $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0
	mv $< /usr/lib64/
	ln -sf /usr/lib64/$(notdir $<) /usr/lib64/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib64/$(notdir $<) /usr/lib64/libbsg_manycore_runtime.so
	cp -t /usr/include $(LIB_HEADERS)

_TARGETS +="uninstall"
_DOCSTRING += "    uninstall:\n"
_DOCSTRING += "        - Remove the installed libraries\n"
uninstall: clean
	sudo rm -f /usr/lib64/libbsg_manycore_* /usr/include/bsg_manycore*.h

.PHONY: platform.clean install uninstall
platform.clean:
	rm -f $(PLATFORM_OBJECTS)

libraries.clean: platform.clean

_TARGETS += $(TARGETS)
TARGETS := $(_TARGETS)

_DOCSTRING += $(DOCSTRING)
DOCSTRING := $(_DOCSTRING)

