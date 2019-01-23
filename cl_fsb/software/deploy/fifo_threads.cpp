#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <iostream>

/* C++ libraries */
#include <vector>
#include <thread>
#include <mutex>
#include <cstdlib>

#include "deploy.h"
#include "../device.h"
#include "fifo.h"

/* concurrency */
#include "concurrentqueue.h"

using namespace std;
using namespace moodycamel;

static const int DW_PER_FSB = 4;
void test_fifos();
void loop_test(int n, void (*f) ());
vector<ConcurrentQueue<pair<uint8_t, int *>>> requests(10); /* FPGA requests across threads */
mutex print_mutex;


namespace FIFO {
	uint8_t READ = 0;
	uint8_t WRITE = 1;
};

/*
 * handles requests for fifo.
 * */
void handle_request (uint8_t fifo) {
	while (true) {
		pair<uint8_t, int *> request;
		bool valid = requests[fifo].try_dequeue(request);
		if (valid) {
			if (request.first == FIFO::READ) {
				int *data = deploy_read_fifo(fifo);
				if (data) {
					for (int i = 0; i < DW_PER_FSB; i++)
						request.second[i] = data[i];
					request.second[DW_PER_FSB] = 1; /* signal thread that data has been read */
					free(data);
				}	
			}	
			else if (request.first == FIFO::WRITE) {
				deploy_write_fifo(fifo, request.second);
			}
		}
	}
}


void get_random (int *buf) {
	if (buf) {
		for (int i = 0; i < DW_PER_FSB; i++) {
			buf[i] = rand();  
		} 
	}
}

bool is_equal (int *read, int *write) {
	if (read && write) {
		bool pass  = true;
		for (int i = 0; i < DW_PER_FSB; i++) {
			if (read[i] != write[i]) {
				pass = false;
				break;
			}
		}
		return pass;
	}
	else 
		return false;
}


/* writes data to fifo n and reads it back in a loop */
void user (int n) {
	int *write = (int *) calloc(DW_PER_FSB, sizeof(int));
	while (true) {
		get_random(write); 
		int *read = (int *) calloc(DW_PER_FSB + 1, sizeof(int));
		requests[n].enqueue(make_pair(FIFO::WRITE, write)); /* issue a write request */
		requests[n].enqueue(make_pair(FIFO_READ, read)); /* issue a read request */
		while (!read[DW_PER_FSB]) {} /* wait for read to be completed. TODO: sleep instead of spinlocking */	
		if (is_equal(read, write)) {/* compare read and write data */
			print_mutex.lock();
			cout << "Pass!" << endl;
			print_mutex.unlock();
		}
		else {
			print_mutex.lock();
			cout << "Fail!" << endl;
			print_mutex.unlock();
		}
		free(read);
	}
}

int main () {
	
	cout << "Running concurrency fifo tests." << endl;

	/* Setup host */
	struct Host *host = (struct Host *) malloc(sizeof(struct Host));	 
	deploy_init_host(host, 0, 0); // DMA arguments unused	

	/* mmap the OCL BAR */
	char *ocl_base = deploy_mmap_ocl();
	if (ocl_base == 0) {
		printf("Error when mmap'ing OCL Bar.\n");
		return 0;
	}

	/* spawn service threads */
	vector<thread> service_threads;
	for (int i = 0; i < NUM_FIFO; i++) {
		service_threads.push_back(thread(handle_request, i));
		service_threads[i].join();
	}	


	/* spawn user threads */
	vector<thread> users;
	for (int i = 0; i < 2; i++) {
		users.push_back(thread(user, i));
		users[i].join();
	} 	

	return 0;
}


