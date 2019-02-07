CXXFLAGS:= -std=c++11 

test: test.o spmd_loader.h
	g++ test.o -o test 

docs:
	doxygen

clean:
	rm -rf *.o test docs/ 

.PHONY: all clean docs
