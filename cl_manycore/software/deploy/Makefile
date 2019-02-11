
INCLUDES = -I$(SDK_DIR)/userspace/include
INCLUDES += -I$(HDK_DIR)/common/software/include 
INCLUDES += -I$(CONCURRENT_QUEUE)

CFLAGS = -std=c11 $(INCLUDES) 
CXXFLAGS = -std=c++11 -pthread -O0 $(INCLUDES)

CC = gcc

fifo_threads: fifo_threads.o deploy.h 
	g++ $(CXXFLAGS) fifo_threads.o -o fifo_threads $(INCLUDES)

loopback_test: loopback_test.o deploy.h 
	gcc $(CFLAGS) loopback_test.o -o loopback_test $(INCUDES)

fifo_test: fifo_test.o deploy.h
	gcc $(CFLAGS) fifo_test.o -o fifo_test $(INCUDES)

dma_dest: dma_test.o deploy.h
	gcc $(CFLAGS) dma_test.o -o dma_test $(INCUDES)

hw_test: hw_test.o 
	gcc $(CFLAGS) hw_test.o -o hw_test $(INCUDES)

clean:
	rm -f *.o dma_test fifo_test hw_test fifo_threads loopback_test
