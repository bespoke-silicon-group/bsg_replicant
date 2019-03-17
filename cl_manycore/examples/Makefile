#INCLUDES = -I../drivers/ -I../libraries/
CFLAGS = -std=c11
CXXFLAGS = -std=c++11 -pthread -O0 
CC = gcc

help: 
	@echo "How to run a Manycore program </dir/main.riscv> quick start guide"
	@echo "    1) write a host program <host_program.c> as follows using the skeleton provided in run_manycore_program.c: "
	@echo "    2) add a target <host_program> to this Makefile."
	@echo "    3) run program with ./<host_program>"

all: loopback_test read_write_test load_vector_test

loopback_test: 
	$(CC) $(CFLAGS) -L/usr/lib/bsg_manycore -lbsg_manycore_runtime loopback_test.c -o loopback_test 

read_write_test: 
	$(CC) $(CFLAGS) -L/usr/lib/bsg_manycore -lbsg_manycore_runtime read_write_test.c -o read_write_test

load_vector_test: 
	$(CC) $(CFLAGS) -L/usr/lib/bsg_manycore -lbsg_manycore_runtime load_vector_test.c -o load_vector_test

clean:
	rm -f *.o loopback_test read_write_test load_vector_test

.PHONY: all clean help
