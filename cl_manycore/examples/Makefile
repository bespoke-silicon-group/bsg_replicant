#INCLUDES = -I../drivers/ -I../libraries/
CFLAGS = -std=c11
CXXFLAGS = -std=c++11 -pthread -O0 
CC = gcc

all: loopback_test read_write_test load_vector_test

loopback_test: 
	$(CC) $(CFLAGS) -L/usr/lib/bsg_manycore -lbsg_manycore_runtime loopback_test.c -o loopback_test 

read_write_test: 
	$(CC) $(CFLAGS) -L/usr/lib/bsg_manycore -lbsg_manycore_runtime read_write_test.c -o read_write_test

load_vector_test: 
	$(CC) $(CFLAGS) -L/usr/lib/bsg_manycore -lbsg_manycore_runtime load_vector_test.c -o load_vector_test

clean:
	rm -f *.o loopback_test read_write_test load_vector_test

.PHONY: all clean
