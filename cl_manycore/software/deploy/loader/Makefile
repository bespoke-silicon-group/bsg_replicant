FESVR2=$(FESVR)
CXXFLAGS:= -std=c++11 -I $(FESVR2)

test: test.o spmd_loader.h
	g++ test.o -o test 
clean:
	rm *.o test
