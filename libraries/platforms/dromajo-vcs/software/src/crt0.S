# Copyright (c) 2020, University of Washington All rights reserved.
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

# Start code

# This file should be deprecated when the BlackParrot firmware is capable of
# parsing arguments.

# This assembly file mimics the BlackParrot perch library start.S
# with minor modifications to allow arguments to be loaded into the
# argument registers for main.

.section ".text.init"
.globl _start
.type  _start, @function
_start:
# Setup the global pointer
.option push
.option norelax
  la gp, _gp
.option pop

# Setup stack pointers.
# We then subtract off 8K*coreID. The top 4K is for the core emulation stack
# the lower 4K is for the program
  la   sp, _sp
  csrr t0, mhartid
  slli t0, t0, 13
  sub  sp, sp, t0

# Save the stack pointer to mscratch, so it can be used on first trap entry
  csrw mscratch, sp
# Get 4K and subtract it off to get the program stack pointer
  li   t0, 1
  slli t0, t0, 12
  sub  sp, sp, t0

# Setup mtvec
  la t0, __mtvec_handler
  csrw mtvec, t0

# Enable FPU
  li t0, 0x00006000 & (0x00006000 >> 1);
  csrs mstatus, t0;
  csrwi fcsr, 0

# 0 all registers except sp and gp
  addi x1,  x0, 0
  addi x4,  x0, 0
  addi x5,  x0, 0
  addi x6,  x0, 0
  addi x7,  x0, 0
  addi x8,  x0, 0
  addi x9,  x0, 0
  addi x10, x0, 0
  addi x11, x0, 0
  addi x12, x0, 0
  addi x13, x0, 0
  addi x14, x0, 0
  addi x15, x0, 0
  addi x16, x0, 0
  addi x17, x0, 0
  addi x18, x0, 0
  addi x19, x0, 0
  addi x20, x0, 0
  addi x21, x0, 0
  addi x22, x0, 0
  addi x23, x0, 0
  addi x24, x0, 0
  addi x25, x0, 0
  addi x26, x0, 0
  addi x27, x0, 0
  addi x28, x0, 0
  addi x29, x0, 0
  addi x30, x0, 0
  addi x31, x0, 0

  la      a0, __libc_fini_array # Register global termination functions
  call    atexit                #  to be called upon exit
  call    __libc_init_array     # Run global initialization functions

#ifdef _DRAMFS
  call dramfs_init
#endif

# Initialize the arguments
  call hb_bp_init_args
  la a0, hb_bp_argc
  ld a0, 0(a0)
  la a1, hb_bp_argv
  ld a1, 0(a1)

  call main
  tail bp_finish

# Loop if we pass exit
loop:
  nop
  j loop
