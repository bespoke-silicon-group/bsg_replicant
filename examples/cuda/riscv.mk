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

# riscv.mk is a method to compile manycore/RISC-V code in
# bsg_replicant so that the host code can co-exist with the kernel
# source. This removes the need to file two PRs (one for this
# repository and one for bsg_manycore) when adding applications. In
# short, it eases the development of kernels, and explaining how to
# develop kernels.

# RISC-V sources are compiled into .rvo files, and then linked into
# .riscv ELF binaries. .o files are for host/x86 code and RISC-V code
# should not be compiled into .o files (that will lead to compiler and
# linker errors.

# To chose between GCC and CLANG set RISCV_CC to RISCV_GCC or RISCV_CLANG, respectively.
# To chose between G++ and CLANG++ set RISCV_CXX to RISCV_GXX or RISCV_CLANGXX, respectively.
# This can be done on a per-object basis, or globally. For example:
# 
# Globally:
# RISCV_CXX = $(RISCV_CLANGXX)
#
# Per-object:
# foo_cpp.rvo: RISCV_CXX=$(RISCV_CLANGXX)

# ****************************** NOTE ******************************
#
# ***** YOU MUST USE = WHEN SETTING RISCV_CC OR RISCV_CXX. DO NOT USE := *****
#
# Using := will cause the automatic variables in the functions above
# to be evaluated before the rule is executed, and they will appear as
# empty spaces when the RISC-V object is compiled.
#
# ****************************** NOTE ******************************

# For an example see test_credit_csr_tile/Makefile.

# riscv.mk was originally in the "baseline" repository, but
# maintaining that repository is difficult when there's a lot of code
# churn, so we brought riscv.mk here and deprecated baseline. 

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

################################################################################
# Paths
################################################################################
_REPO_ROOT ?= $(shell git rev-parse --show-toplevel)
-include $(_REPO_ROOT)/environment.mk
-include $(BSG_MACHINE_PATH)/Makefile.machine.include

BSG_MANYCORE_SPMD_PATH = $(BSG_MANYCORE_DIR)/software/spmd/
BSG_MANYCORE_CUDALITE_PATH = $(BSG_MANYCORE_SPMD_PATH)/bsg_cuda_lite_runtime/
BSG_MANYCORE_CUDALITE_MAIN_PATH = $(BSG_MANYCORE_CUDALITE_PATH)/main

BSG_MANYCORE_LIB_PATH    = $(BSG_MANYCORE_DIR)/software/bsg_manycore_lib
BSG_MANYCORE_COMMON_PATH = $(BSG_MANYCORE_SPMD_PATH)/common/

RISCV_TOOLS_PATH := $(BSG_MANYCORE_DIR)/software/riscv-tools/
RISCV_GNU_PATH   := $(RISCV_TOOLS_PATH)/riscv-install
RISCV_LLVM_PATH  := $(RISCV_TOOLS_PATH)/llvm/llvm-install

################################################################################
# RISC-V Tool Configuration
################################################################################

RISCV_LINK_GEN ?= $(BSG_MANYCORE_DIR)/software/py/bsg_manycore_link_gen.py

# These flags are not supported by clang
RISCV_GNU_FLAGS = -frerun-cse-after-loop -fweb -frename-registers -mtune=bsg_vanilla_2020

# The preceeding underscore is used to provide single-invocation like
# semantics for clang. See comment below about _RISCV_GCC and
# _RISCV_GXX for more explanation.
_RISCV_GCC       ?= $(RISCV_GNU_PATH)/bin/riscv32-unknown-elf-dramfs-gcc $(RISCV_GNU_FLAGS)
_RISCV_GXX       ?= $(RISCV_GNU_PATH)/bin/riscv32-unknown-elf-dramfs-g++ $(RISCV_GNU_FLAGS)
RISCV_ELF2HEX    ?= LD_LIBRARY_PATH=$(RISCV_GNU_PATH)/lib $(RISCV_GNU_PATH)/bin/elf2hex
RISCV_OBJCOPY    ?= $(RISCV_GNU_PATH)/bin/riscv32-unknown-elf-dramfs-objcopy
RISCV_AR         ?= $(RISCV_GNU_PATH)/bin/riscv32-unknown-elf-dramfs-ar
RISCV_OBJDUMP    ?= $(RISCV_GNU_PATH)/bin/riscv32-unknown-elf-dramfs-objdump
RISCV_LINK       ?= $(_RISCV_GCC) -t -T $(LINK_SCRIPT) $(RISCV_LDFLAGS)
RISCV_LD         ?= $(_RISCV_GCC)

RISCV_CLANG_ABI        = ilp32f
RISCV_CLANG_CCPPFLAGS += --target=riscv32 -mtune=hb-rv32 -mcpu=hb-rv32 -mabi=$(RISCV_CLANG_ABI)
RISCV_CLANG_CXXFLAGS  += --sysroot=$(RISCV_GNU_PATH)/riscv32-unknown-elf-dramfs
RISCV_CLANG_CXXFLAGS  += -I$(RISCV_GNU_PATH)/riscv32-unknown-elf-dramfs/include/c++/9.2.0
RISCV_CLANG_CXXFLAGS  += -I$(RISCV_GNU_PATH)/riscv32-unknown-elf-dramfs/include/c++/9.2.0/riscv32-unknown-elf-dramfs
RISCV_LLVM_LLC_FLAGS   = -march=riscv32 -mcpu=hb-rv32 -mattr=+m,+a,+f

# See comment below about _RISCV_CLANG and _RISCV_CLANGXX for
# explanation of the preceding underscore.
_RISCV_CLANG       ?= $(RISCV_LLVM_PATH)/bin/clang $(RISCV_CLANG_CFLAGS) $(RISCV_CLANG_CCPPFLAGS)
_RISCV_CLANGXX     ?= $(RISCV_LLVM_PATH)/bin/clang++ $(RISCV_CLANG_CXXFLAGS) $(RISCV_CLANG_CCPPFLAGS)
RISCV_LLVM_OPT    ?= $(RISCV_LLVM_PATH)/bin/opt
RISCV_LLVM_LLC    ?= $(RISCV_LLVM_PATH)/bin/llc
RISCV_LLVM_LIB    ?= $(RISCV_LLVM_PATH)/lib

# When compiling RISC-V code we often compile part of a program with
# LLVM and part of a program with GCC because sometimes one compiler
# does better than the other. 

# In a typical x86 flow you can change your C/C++ compiler by setting
# CC or CXX like this:

# Globally:

# CC=clang
# OR
# CXX=clang++

# Per-object:

# foo.o: CC=clang
# OR
# foo_cpp.o: CXX=clang++

# This behavior is not currently an option for HB RISC-V (as of
# 7/1/2021) because llc (the internal optimizer for LLVM) doesn't
# infer the correct scheduling weights for important HB features like
# bsg_attr_remote because -mcpu and other flags are not passed from
# clang (the front end). See:
# https://github.com/bespoke-silicon-group/bsg_manycore/blob/3a05e000aef1fba896f756e9836c9e002e5a06f7/software/mk/Makefile.builddefs#L208

# As a result clang/LLVM compilation is split into
# multiple steps, instead of a single invocation:
#   1. Clang (.c/.cpp -> .ll (LLVM IR))
#   2. llc (.ll (LLVM IR) -> .s)
#   3. gcc/clang (.s -> .rvo)

# This is problematic if we want to selectively compile objects with
# clang/LLVM because it adds intermediate targets that GCC does not
# have. AFAIK there is no easy way to express two different
# compilation paths in a single run of make. If clang correctly passed
# the arguments to lcc and HB RV32 objects could be optimally compiled
# with a single clang command, this issue would be resolved and we
# could use a flow like the x86 example above.

# The current approach in bsg_manycore does not provide the ability to
# do per-object compiler selection. CLANG=1 is set globally, which
# compiles ALL application objects with LLVM, or none of them.

# Hence, this solution. The approach below seems to be the only way to
# provide per-object selection of GCC/LLVM with our current flow.
#
# Instead of having a single invocation of clang, we wrap clang/LLVM
# into a makefile function to provide the abstraction of a single
# command. These functions are RISCV_GCC/RISCV_GXX or
# RISCV_CLANG/RISCV_CLANGXX for C and C++, respectively.
#
# Providing RISCV_GCC/RISCV_GXX and RISCV_CLANG/RISCV_CLANGXX as
# callable functions provides the abstraction of being able to choose
# the compiler on a per-object basis (e.g. kernel.rvo:RISCV_CC=RISC_GCC) or globally.

# For an example of how to use this file see
# test_credit_csr_tile/Makefile.

RISCV_GCC = $(_RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@ |& tee $*.rvo.log
RISCV_GXX = $(_RISCV_GXX) $(RISCV_CXXFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@ |& tee $*.rvo.log

# The clang version of the functions hide the fact that LLC (the
# optimizer) is being called between steps.
define RISCV_CLANGXX
$(_RISCV_CLANGXX) $(RISCV_CXXFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@.ll -S -emit-llvm && \
$(RISCV_LLVM_LLC) $(RISCV_LLVM_LLC_FLAGS) $@.ll -o $@.S && \
$(_RISCV_CLANG) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $@.S -o $@ |& tee $*.rvo.log
endef 

define RISCV_CLANG
$(_RISCV_CLANG) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@.ll -S -emit-llvm |& tee $*.rvo.ll.log && \
$(RISCV_LLVM_LLC) $(RISCV_LLVM_LLC_FLAGS) $@.ll -o $@.S |& tee $*.rvo.S.log && \
$(_RISCV_CLANG) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $@.S -o $@ |& tee $*.rvo.log
endef

# Set the default RISC-V Compilers.
RISCV_CXX ?= $(RISCV_GXX)
RISCV_CC  ?= $(RISCV_GCC)

################################################################################
# C/C++ Compilation Flags
#
# All RISCV C/C++ compilation variables simply have RISCV_* appended.
################################################################################
RISCV_OPT_LEVEL   ?= -O2
RISCV_ARCH_OP     := rv32imaf

# CCPPFLAGS are common between GCC and G++
RISCV_CCPPFLAGS += $(RISCV_OPT_LEVEL)
RISCV_CCPPFLAGS += -march=$(RISCV_ARCH_OP)
RISCV_CCPPFLAGS += -g
RISCV_CCPPFLAGS += -static
RISCV_CCPPFLAGS += -ffast-math
RISCV_CCPPFLAGS += -fno-common
RISCV_CCPPFLAGS += -ffp-contract=off

RISCV_CFLAGS   += -std=gnu99 $(RISCV_CCPPFLAGS)
RISCV_CXXFLAGS += -std=c++11 $(RISCV_CCPPFLAGS)
RISCV_CXXFLAGS += -fno-threadsafe-statics

RISCV_INCLUDES += -I$(BSG_MANYCORE_COMMON_PATH)
RISCV_INCLUDES += -I$(BSG_MANYCORE_DIR)/software/bsg_manycore_lib

# Check for machine definitions. These are typically set by
# Makefile.machine.include

ifndef BSG_MACHINE_GLOBAL_X
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_GLOBAL_X undefined"))
endif
ifndef BSG_MACHINE_GLOBAL_Y
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_GLOBAL_Y undefined"))
endif
RISCV_DEFINES += -Dbsg_global_X=$(BSG_MACHINE_GLOBAL_X)
RISCV_DEFINES += -Dbsg_global_Y=$(BSG_MACHINE_GLOBAL_Y)

ifndef BSG_MACHINE_POD_TILES
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_POD_TILES undefined"))
endif
ifndef BSG_MACHINE_PODS_X
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_PODS_X undefined"))
endif
ifndef BSG_MACHINE_PODS_Y
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_PODS_Y undefined"))
endif
RISCV_DEFINES += -Dbsg_group_size=$(BSG_MACHINE_POD_TILES)
RISCV_DEFINES += -Dbsg_pods_X=$(BSG_MACHINE_PODS_X)
RISCV_DEFINES += -Dbsg_pods_Y=$(BSG_MACHINE_PODS_Y)

ifndef BSG_MACHINE_HOST_X_CORD
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_HOST_X_CORD undefined"))
endif
ifndef BSG_MACHINE_HOST_Y_CORD
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_HOST_Y_CORD undefined"))
endif
RISCV_DEFINES += -DIO_X_INDEX=$(BSG_MACHINE_HOST_X_CORD)
RISCV_DEFINES += -DIO_Y_INDEX=$(BSG_MACHINE_HOST_Y_CORD)
RISCV_DEFINES += -DPREALLOCATE=0
RISCV_DEFINES += -DHOST_DEBUG=0

crt.rvo: $(BSG_MANYCORE_COMMON_PATH)/crt.S
	$(_RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@ |& tee $*.rvo.log

# We compile these locally so that we don't interfere with the files in
# $(BSG_MANYCORE_LIB_PATH).
# BSG Manycore Library Objects
LIBBSG_MANYCORE_OBJECTS  += bsg_set_tile_x_y.c.rvo
LIBBSG_MANYCORE_OBJECTS  += bsg_tile_config_vars.c.rvo
LIBBSG_MANYCORE_OBJECTS  += bsg_printf.c.rvo
LIBBSG_MANYCORE_OBJECTS  += bsg_barrier_amoadd.S.rvo
LIBBSG_MANYCORE_OBJECTS  += bsg_cuda_lite_barrier.c.rvo

libbsg_manycore_riscv.a: $(LIBBSG_MANYCORE_OBJECTS)
	$(RISCV_AR) rcs $@ $^

# See comment above about _RISCV_GCC and _RISCV_GXX for explanation of
# the preceding underscore.
$(LIBBSG_MANYCORE_OBJECTS) main.rvo: RISCV_CXX = $(_RISCV_GCC)

$(filter %.c.rvo,$(LIBBSG_MANYCORE_OBJECTS)): %.c.rvo:$(BSG_MANYCORE_LIB_PATH)/%.c
	$(_RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@

$(filter %.S.rvo,$(LIBBSG_MANYCORE_OBJECTS)): %.S.rvo:$(BSG_MANYCORE_LIB_PATH)/%.S
	$(_RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_DEFINES) -D__ASSEMBLY__=1 $(RISCV_INCLUDES) -c $< -o $@

main.rvo: $(BSG_MANYCORE_CUDALITE_MAIN_PATH)/main.c
	$(_RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_DEFINES) $(RISCV_INCLUDES) -c $< -o $@

%.rvo: %.c
	$(call RISCV_CC)

%.rvo: %.cpp
	$(call RISCV_CXX)

kernel.compile.clean:
	rm -rf *.rvo *.a

.PRECIOUS: %.rvo

################################################################################
# Linker Flow
################################################################################

# ELF File Parameters
# Default .data section location; LOCAL=>DMEM, SHARED=>DRAM.
BSG_ELF_DEFAULT_DATA_LOC ?= LOCAL

BSG_ELF_OFF_CHIP_MEM := $(BSG_MACHINE_DRAM_INCLUDED)

# Total addressable DRAM size (in 32-bit WORDS, and SIZE bytes)
BSG_ELF_DRAM_WORDS := $(shell expr $(BSG_MACHINE_DRAM_BANK_SIZE_WORDS) \* $(BSG_MACHINE_GLOBAL_X))
BSG_ELF_DRAM_SIZE := $(shell expr $(BSG_ELF_DRAM_WORDS) \* 4)

# Victim Cache Set Size (in 32-bit WORDS and SIZE bytes)
_BSG_ELF_VCACHE_SET_WORDS := $(shell expr $(BSG_MACHINE_VCACHE_WAY) \* $(BSG_MACHINE_VCACHE_BLOCK_SIZE_WORDS))
BSG_ELF_VCACHE_SET_SIZE := $(shell expr $(_BSG_ELF_VCACHE_SET_WORDS) \* 4)

# Victim Cache Column Size (in 32-bit WORDS and SIZE bytes)
_BSG_ELF_VCACHE_COLUMN_WORDS := $(shell expr $(BSG_MACHINE_VCACHE_SET) \* $(_BSG_ELF_VCACHE_SET_WORDS))
BSG_ELF_VCACHE_COLUMN_SIZE := $(shell expr $(_BSG_ELF_VCACHE_COLUMN_WORDS) \* 4)

# Victim Cache Total Size (in 32-bit WORDS, and SIZE BYTES)
_BSG_ELF_VCACHE_MANYCORE_WORDS ?= $(shell expr $(BSG_MACHINE_GLOBAL_X) \* $(_BSG_ELF_VCACHE_COLUMN_WORDS))
BSG_ELF_VCACHE_MANYCORE_SIZE := $(shell expr $(_BSG_ELF_VCACHE_MANYCORE_WORDS) \* 4)

# Compute the ELF Stack Pointer Location.  
ifeq ($(BSG_ELF_DEFAULT_DATA_LOC), LOCAL)
# If the .data segment is in DMEM (LOCAL) then put it at the top of DMEM. (This is the typical case)
BSG_ELF_STACK_PTR ?= 0x00000ffc
else
  # EVA Offset in DRAM
  BSG_ELF_DRAM_EVA_OFFSET = 0x80000000

  ifeq ($(BSG_ELF_OFF_CHIP_MEM), 1)
  # Otherwise, use the top of DRAM (if present),
  _BSG_ELF_DRAM_LIMIT = $(shell expr $(BSG_ELF_DRAM_EVA_OFFSET) + $(BSG_ELF_DRAM_SIZE))
  else
  # Or the Victim Cache address space (if DRAM is disabled/not present).
  _BSG_ELF_DRAM_LIMIT = $(shell expr $(BSG_ELF_DRAM_EVA_OFFSET) + $(BSG_ELF_VCACHE_MANYCORE_SIZE))
  endif
# Finally, Subtract 4 from the maximum memory space address
BSG_ELF_STACK_PTR = $(shell expr $(_BSG_ELF_DRAM_LIMIT) - 4)
endif

# Linker script generation parameters
ifeq ($(BSG_ELF_OFF_CHIP_MEM), 1)
  ifeq ($(BSG_ELF_DEFAULT_DATA_LOC), LOCAL)
    LINK_GEN_OPTS ?= --default_data_loc=dmem --dram_size=$(BSG_ELF_DRAM_SIZE) --sp=$(BSG_ELF_STACK_PTR)
  else ifeq ($(BSG_ELF_DEFAULT_DATA_LOC), SHARED)
    LINK_GEN_OPTS ?= --default_data_loc=dram --dram_size=$(BSG_ELF_DRAM_SIZE) --sp=$(BSG_ELF_STACK_PTR)
  else
    $(error Invalid BSG_ELF_DEFAULT_DATA_LOC = $(BSG_ELF_DEFAULT_DATA_LOC); Only LOCAL and SHARED are valid)
  endif

  LINK_GEN_OPTS += --imem_size=0x01000000 # 16MB
else ifeq ($(BSG_ELF_OFF_CHIP_MEM), 0)
  ifeq ($(BSG_ELF_DEFAULT_DATA_LOC), LOCAL)
    LINK_GEN_OPTS ?= --default_data_loc=dmem --dram_size=$(BSG_ELF_VCACHE_SIZE) --sp=$(BSG_ELF_STACK_PTR)
  else ifeq ($(BSG_ELF_DEFAULT_DATA_LOC), SHARED)
    LINK_GEN_OPTS ?= --default_data_loc=dram --dram_size=$(BSG_ELF_VCACHE_SIZE) --sp=$(BSG_ELF_STACK_PTR)
  else
    $(error Invalid BSG_ELF_DEFAULT_DATA_LOC = $(BSG_ELF_DEFAULT_DATA_LOC); Only LOCAL and SHARED are valid)
  endif

  LINK_GEN_OPTS += --imem_size=0x00008000 # 32KB
else
  $(error Invalid BSG_ELF_OFF_CHIP_MEM = $(BSG_ELF_OFF_CHIP_MEM); Only 0 and 1 are valid)
endif

RISCV_LINK_SCRIPT ?= bsg_link.ld
$(RISCV_LINK_SCRIPT): $(RISCV_LINK_GEN)
	$(RISCV_LINK_GEN) $(LINK_GEN_OPTS) --out=$@

# Link commands and definitions

RISCV_LDFLAGS += -Wl,--defsym,_bsg_elf_dram_size=$(BSG_ELF_DRAM_SIZE)
RISCV_LDFLAGS += -Wl,--defsym,_bsg_elf_vcache_size=$(BSG_ELF_VCACHE_MANYCORE_SIZE)
RISCV_LDFLAGS += -Wl,--defsym,_bsg_elf_stack_ptr=$(BSG_ELF_STACK_PTR)

RISCV_LDFLAGS += -nostdlib
RISCV_LDFLAGS += -march=$(RISCV_ARCH_OP)
RISCV_LDFLAGS += -nostartfiles
RISCV_LDFLAGS += -ffast-math
RISCV_LDFLAGS += -lc
RISCV_LDFLAGS += -lm
RISCV_LDFLAGS += -lgcc
RISCV_LDFLAGS += -L.
RISCV_LDFLAGS += -lbsg_manycore_riscv

# TODO: temporary fix to solve this problem: https://stackoverflow.com/questions/56518056/risc-v-linker-throwing-sections-lma-overlap-error-despite-lmas-belonging-to-dif
RISCV_LDFLAGS += -Wl,--no-check-sections 

# This builds a .riscv binary for the current machine type and tile
# group size. RISCV_TARGET_OBJECTS are .rvo files that will be linked
# in the final binary.
%.riscv: crt.rvo libbsg_manycore_riscv.a main.rvo $(RISCV_TARGET_OBJECTS) $(RISCV_LINK_SCRIPT) 
	$(RISCV_LD) -T $(RISCV_LINK_SCRIPT) $(filter %.rvo,$^) -o $@ $(RISCV_LDFLAGS) 

%.dis: %.riscv
	$(RISCV_OBJDUMP) -dS $<

kernel.link.clean:
	rm -rf *.riscv *.rvo.S *.rvo.ll $(RISCV_LINK_SCRIPT) libbsg_manycore_riscv.a


.PRECIOUS: %.riscv
.PHONY: kernel.link.clean kernel.compile.clean
clean: kernel.link.clean kernel.compile.clean
