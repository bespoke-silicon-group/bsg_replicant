#INCLUDES = -I../drivers/ -I../libraries/
CFLAGS  := -std=c11
CXXFLAGS:= -std=c++11 -pthread -O0
LDFLAGS := -lbsg_manycore_runtime


# if you have written a program such as <my-program>.c or <my-program>.cpp
# add <my-program> in the appropriate list of targets below
C_TARGETS   := test_dmem_read_write test_rom
C_BINARY_TARGETS := test_bsg_dram_loopback_cache
CXX_TARGETS := 

TARGETS := $(C_TARGETS) $(CXX_TARGETS) $(C_BINARY_TARGETS)
HEADERS := hb_mc_print_hex.h

all: $(TARGETS)

# all objects depend on all headers
$(wilcard *.o): $(HEADERS)

# each target 't' in TARGETS relies on an object file 't.o'
$(TARGETS): %: %.o

# ensure correct linking by using C/C++ linker frontend
$(C_TARGETS) $(C_BINARY_TARGETS):   LD:=$(CC)
$(CXX_TARGETS): LD:=$(CXX)

bsg_tiles_X ?= 4
bsg_tiles_Y ?= 4


# linking rule for all targets
$(TARGETS):
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS) 

NOISY ?= 1

REGRESSION_BINARY := $(foreach target, $(C_BINARY_TARGETS), $(subst test_,,$(target)).binary)

regression: $(REGRESSION_BINARY)
	@pass=0;  total=0; \
	for target in $(C_TARGETS) $(C_BINARY_TARGETS); do \
		if make $$target.regression; then \
			let "pass+=1"; \
		fi; \
		let "total+=1"; \
	done; \
	echo $$pass out of $$total tests pass.;


%.binary:
	make -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst .binary,,$@) clean
	make -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst .binary,,$@) main.riscv bsg_tiles_X=$(bsg_tiles_X) bsg_tiles_Y=$(bsg_tiles_Y)

%.binary_clean:
	make -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst test_,,$(subst .binary_clean,,$@)) clean

ifeq ($(NOISY), 1)
%.regression: %
	@echo "=========================================================="
	@echo "start running $<: "
	@echo "=========================================================="
	@sudo -E ./$<
	@echo ""
else
%.regression: %
	@echo "=========================================================="
	@echo "start running $<: "
	@sudo -E ./$< > tmp.log
	@grep "PASSED\|FAILED" tmp.log
	@echo "=========================================================="
	@echo ""	
endif

clean: $(foreach target, $(C_BINARY_TARGETS), $(target).binary_clean)
	rm -f *.o $(TARGETS) tmp.log

.PHONY: all clean %.regression
