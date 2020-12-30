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

# This Makefile fragment defines rules for compilation of the C/C++
# files for running regression tests.

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 

.PRECIOUS: %.o

## TODO: Specialize to BP Platform
# BlackParrot GCC
BP_CC = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-gcc
BP_CXX = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-g++
# Usage objdump -d -t <prog.riscv> > <prog.dump>
BP_OBJDUMP = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-objdump
# Usage objcopy -O verilog <prog.riscv> <prog.mem>
BP_OBJCOPY = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-objcopy
# Usage python nbf.py --config --ncpus=1 --mem=<prog.mem> > <prog.nbf>
BP_NBF = $(BLACKPARROT_DIR)/bp_common/software/py/nbf.py
PYTHON = python

# each regression target needs to build its .o from a .c and .h of the
# same name
%.o: %.c %.h
	$(CC) -c -o $@ test_bsg_scalar_print.c $(INCLUDES) $(CFLAGS) $(CDEFINES) -DBSG_TEST_NAME=$(patsubst %.c,%,$<) 
	touch $@
	$(BP_CC) -o $*.rv64o $< $(INCLUDES) $(CFLAGS) $(CDEFINES) -DBSG_TEST_NAME=$(patsubst %.c,%,$<) \
		-march=rv64ima -mabi=lp64 -mcmodel=medany \
		-static -nostartfiles -L$(BLACKPARROT_DIR)/bp_common/test/lib/ -lperch -Triscv.ld -UVCS -fPIC
	$(BP_OBJDUMP) -d -t $*.rv64o > prog.dump
	$(BP_OBJCOPY) -O verilog $*.rv64o prog.mem
	#$(PYTHON) $(BP_NBF) --config --ncpus=1 --mem=prog.mem > prog.nbf

# ... or a .cpp and .hpp of the same name
%.o: %.cpp %.hpp
	#$(CXX) -c -o $@ $< $(INCLUDES) $(CXXFLAGS) $(CXXDEFINES) -DBSG_TEST_NAME=$(patsubst %.cpp,%,$<) 
	touch $@
	$(BP_CXX) -c -o $*.rv64o $< $(INCLUDES) $(CXXFLAGS) $(CXXDEFINES) -DBSG_TEST_NAME=$(patsubst %.cpp,%,$<) 
	$(BP_OBJDUMP) -d -t $*.rv64o > prog.dump
	$(BP_OBJCOPY) -O verilog $*.rv64o prog.mem
	#$(PYTHON) $(BP_NBF) --config --ncpus=1 --mem=prog.mem > prog.nbf

.PHONY: platform.compilation.clean
platform.compilation.clean:
	rm -rf *.o

compilation.clean: platform.compilation.clean
