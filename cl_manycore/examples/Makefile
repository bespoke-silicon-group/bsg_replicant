#INCLUDES = -I../drivers/ -I../libraries/
CFLAGS  := -std=c11
CXXFLAGS:= -std=c++11 -pthread -O0
LDFLAGS := -lbsg_manycore_runtime


# if you have written a program such as <my-program>.c or <my-program>.cpp
# add <my-program> to the list of targets below
TARGETS := loopback_test read_write_test load_vector_test

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean 
