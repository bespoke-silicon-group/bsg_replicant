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


/*
 * Right now:
 *	 U = {u_1, ..., u_m} that can access any fifo
 *	 C = {c_1, ..., c_n} that complete requests
 * Change:
 *	U = {u_1, ...,u_m} can only access its fifos
 *		error check threads vying for same fifo (later)
 *	u_i
 *		requests fifos
 *		calls deploy_write and deploy_read to them 
 * */


/* Helper testing functions */
namespace FIFO {
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
			for (int i = 0; i < 2; i++) {
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
	int NUM_PACKETS = 1;
	uint8_t NUM_USERS = 10;
};


/* writes data to fifo n and reads it back in a loop 
 * writes 1 128b packet to FPGA and then reads it back
 * */
void user (int n) {
	
	vector<int *> writes;
	bool valid; 

	for (int i = 0; i < FIFO::NUM_PACKETS; i++) {
		int *write = (int *) calloc(DW_PER_FSB, sizeof(int));
		FIFO::get_random(write); 
		deploy_write_fifo(i, write); /* write the packets */
		writes.push_back(write);
	}

	bool pass = true;
	for (int fsb_p = 0; fsb_p < FIFO::NUM_PACKETS; fsb_p++) {
		int *read = deploy_read_fifo(n);
		if (!read) {
			printf("fifo test %d failed. No read data for fsb packet %d received.\n", n, fsb_p);
			pass = false;
		}
		else {
			for (int j = 0; j < 2; j++) {
				if (read[j] != writes[fsb_p][j]) {
					printf("fifo test %d failed. Mismatch at fsb packet %d, DW %d: Received %d instead of %x. \n", n, fsb_p, j, read[j], j);
					pass = false;
					break;
				}
			}
		}
		if (read)
			free(read);

	}
	if (pass) {
		printf("fifo test %d passed.\n", n) ;
	}

	for (int i = 0; i < FIFO::NUM_PACKETS; i++)
		free(writes[i]);
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

	/* spawn user threads */
	vector<thread> users;
	for (int i = 0; i < FIFO::NUM_USERS; i++) {
		users.push_back(thread(user, i));
	}
	for (int i = 0; i < FIFO::NUM_USERS; i++) {
		users[i].join();
	} 	
	return 0;
}


