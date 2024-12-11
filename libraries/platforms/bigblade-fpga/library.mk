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

# Append per-platform flags to the library building options
$(LIB_OBJECTS) $(LIB_OBJECTS_CUDA_POD_REPL) $(LIB_OBJECTS_REGRESSION): INCLUDES += -I$(BSG_PLATFORM_PATH)

# bigblade-fpga platform source files
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/platforms/bigblade-fpga/bsg_manycore_mmio.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/platforms/bigblade-fpga/bsg_manycore_platform.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/features/profiler/noimpl/bsg_manycore_profiler.cpp
PLATFORM_CXXSOURCES += $(LIBRARIES_PATH)/features/tracer/noimpl/bsg_manycore_tracer.cpp

# bigblade-fpga does not provide a DMA feature. Therefore, we use the fragment in 
# features/dma/noimpl/feature.mk that simply returns
# HB_MC_NO_IMPL for each function call.
include $(LIBRARIES_PATH)/features/dma/noimpl/feature.mk

PLATFORM_OBJECTS += $(patsubst %cpp,%o,$(PLATFORM_CXXSOURCES))
PLATFORM_OBJECTS += $(patsubst %c,%o,$(PLATFORM_CSOURCES))

PLATFORM_REGRESSION_OBJECTS += $(patsubst %cpp,%o,$(PLATFORM_REGRESSION_CXXSOURCES))
PLATFORM_REGRESSION_OBJECTS += $(patsubst %c,%o,$(PLATFORM_REGRESSION_CSOURCES))

$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES := -I$(LIBRARIES_PATH)
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/platforms/bigblade-fpga
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/dma
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/profiler
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): INCLUDES += -I$(LIBRARIES_PATH)/features/tracer

$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): CFLAGS   := -std=c11 -fPIC -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): CXXFLAGS := -std=c++11 -fPIC -D_GNU_SOURCE -D_BSD_SOURCE -D_DEFAULT_SOURCE
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): LDFLAGS  := -fPIC
$(PLATFORM_OBJECTS) $(PLATFORM_REGRESSION_OBJECTS): LDFLAGS  += -ldl

$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0: $(PLATFORM_OBJECTS)
$(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so.1.0: $(PLATFORM_REGRESSION_OBJECTS)

_DOCSTRING := "Rules from bigblade-fpga/library.mk\n"
_TARGETS :=

_TARGETS +="install"
_DOCSTRING += "    install:\n"
_DOCSTRING += "        - Install libbsg_manycore_runtime.so, libbsg_manycore_regression.so and libbsgmc_cuda_legacy_pod_repl.so in\n"
_DOCSTRING += "          /usr/lib64 and the headers in /usr/include\n"
install: $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so.1.0 \
         $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so.1.0 \
         $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so.1.0
	mv $(word 1,$^) /usr/lib64/
	ln -sf /usr/lib64/$(notdir $(word 1,$^)) /usr/lib64/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib64/$(notdir $(word 1,$^)) /usr/lib64/libbsg_manycore_runtime.so
	mv $(word 2,$^) /usr/lib64/
	ln -sf /usr/lib64/$(notdir $(word 2,$^)) /usr/lib64/libbsg_manycore_regression.so.1
	ln -sf /usr/lib64/$(notdir $(word 2,$^)) /usr/lib64/libbsg_manycore_regression.so
	mv $(word 3,$^) /usr/lib64/
	ln -sf /usr/lib64/$(notdir $(word 3,$^)) /usr/lib64/libbsgmc_cuda_legacy_pod_repl.so.1
	ln -sf /usr/lib64/$(notdir $(word 3,$^)) /usr/lib64/libbsgmc_cuda_legacy_pod_repl.so
	cp -t /usr/include $(LIB_HEADERS)

_TARGETS +="uninstall"
_DOCSTRING += "    uninstall:\n"
_DOCSTRING += "        - Remove the installed libraries\n"
uninstall: clean
	sudo rm -f /usr/lib64/libbsg_manycore_* /usr/lib64/libbsgmc_* /usr/include/bsg_manycore*.h

.PHONY: platform.clean install uninstall
platform.clean:
	rm -f $(PLATFORM_OBJECTS)

libraries.clean: platform.clean

_TARGETS += $(TARGETS)
TARGETS := $(_TARGETS)

_DOCSTRING += $(DOCSTRING)
DOCSTRING := $(_DOCSTRING)

