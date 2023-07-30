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

# This Makefile fragment defines the rules that are used for executing
# applications on HammerBlade Platforms

.PRECIOUS: exec.log saifgen.log
.PHONY: platform.execution.clean

##%.log: test_loader.riscv
##	echo "Test created"
##
##DRAMFS_CSOURCES += $(BSG_PLATFORM_PATH)/src/bsg_newlib_intf.c
##DRAMFS_CSOURCES += lfs.c
##
##DRAMFS_OBJECTS += $(patsubst %cpp,%o,$(DRAMFS_CXXSOURCES))
##DRAMFS_OBJECTS += $(patsubst %c,%o,$(DRAMFS_CSOURCES))
##
##test_loader.riscv: test_loader.o $(DRAMFS_OBJECTS)
##	echo "Trying to build test_loader.riscv"
##	$(CXX) -mcmodel=medany -fPIC -static -T$(BLACKPARROT_SDK_DIR)/install/linker/riscv.ld -o $@ $^
##
###LDFLAGS += -T$(BLACKPARROT_SDK_DIR)/install/linker/riscv.ld
##
### 1 MB
##DRAMFS_MKLFS ?= $(BLACKPARROT_SDK_DIR)/install/bin/dramfs_mklfs 128 8192
##lfs.c: $(BSG_MANYCORE_KERNELS)
##	cp $< $(notdir $(BSG_MANYCORE_KERNELS))
##	$(DRAMFS_MKLFS) $(notdir $(BSG_MANYCORE_KERNELS)) > $@
##
##platform.execution.clean:
##	rm -rf exec.log
##
##execution.clean: platform.execution.clean
