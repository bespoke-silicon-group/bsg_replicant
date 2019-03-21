#INCLUDES = -I../drivers/ -I../libraries/
CFLAGS  := -std=c11
CXXFLAGS:= -std=c++11 -pthread -O0
LDFLAGS := -lbsg_manycore_runtime
TARGETS := loopback_test read_write_test load_vector_test

all: $(TARGETS)

help: 
	@echo "How to run a Manycore program </dir/main.riscv> quick start guide"
	@echo "    1) write a host program <host_program.c> as follows using the skeleton provided in run_manycore_program.c: "
	@echo "    2) add a target <host_program> to this Makefile."
	@echo "    3) run program with ./<host_program>"

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean help
