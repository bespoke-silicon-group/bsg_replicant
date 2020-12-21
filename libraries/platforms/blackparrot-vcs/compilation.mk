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

# This Makefile fragment defines rules/flags for compiling C/C++ files

# BlackParrot GCC
BP_CC = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-gcc
# Usage objdump -d -t <prog.mem> > <prog.dump>
BP_OBJDUMP = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-objdump
# Usage objcopy -O verilog <prog.riscv> <prog.mem>
BP_OBJCOPY = $(BLACKPARROT_DIR)/external/bin/riscv64-unknown-linux-gnu-objcopy
# Usage python nbf.py --config --skip-zeros --ncpus=1 --prog=<prog.mem>
BP_NBF = $(BLACKPARROT_DIR)/bp_common/software/py/nbf.py

# TODO: Need to grab a host program, convert to .mem, convert to .nbf, and
#   place .mem and .nbf in the same directory as the executable
#PROG :=$(BLACKPARROT_DIR)/bp_common/test/mem/bp_tests/hello_world.riscv

# TODO: Need to replace with both x86 and RISC-V compilation, for now.
#   Eventually only RISC-V
include $(LIBRARIES_PATH)/platforms/dpi-vcs/compilation.mk
