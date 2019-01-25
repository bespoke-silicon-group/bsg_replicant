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


using namespace std;

static const int DW_PER_FSB = 4;
void test_fifos();
void loop_test(int n, void (*f) ());
mutex pcie, print_mutex;

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
	int NUM_PACKETS = 10;
	uint8_t NUM_USERS = 10;
	
	int *writes;
	int *reads;
};


/* writes data to fifo n and reads it back in a loop 
 * writes 1 128b packet to FPGA and then reads it back
 * */
void user (int n) {
	bool valid; 
	int *write = FIFO::writes + (n * FIFO::NUM_PACKETS * DW_PER_FSB);	

	
	for (int i = 0; i < FIFO::NUM_PACKETS; i++) {
		pcie.lock();
		deploy_write_fifo(n, write + i * DW_PER_FSB); /* write the packets */
		pcie.unlock();
	}
		
	bool pass = true;
	for (int fsb_p = 0; fsb_p < FIFO::NUM_PACKETS; fsb_p++) {
		pcie.lock();
		int *read = deploy_read_fifo(n, FIFO::reads + (n * FIFO::NUM_PACKETS * DW_PER_FSB + (fsb_p * DW_PER_FSB)));
		pcie.unlock();
		if (!read) {
			print_mutex.lock();
			printf("fifo test %d failed. No read data for fsb packet %d received.\n", n, fsb_p);
			print_mutex.unlock();
			pass = false;
		}
		else {
			for (int j = 0; j < 2; j++) {
				if (read[j] != (write + fsb_p * DW_PER_FSB)[j]) {
					print_mutex.lock();
					printf("fifo test %d failed. Mismatch at fsb packet %d, DW %d: Received %d instead of %d. Address of written DW: %p.\n", n, fsb_p, j, read[j], (write + fsb_p * DW_PER_FSB)[j], write + fsb_p * DW_PER_FSB + j );
					printf("Write base: %p, Read base: %p\n", fifo[n][FIFO_WRITE] & (~ 0xFF), fifo[n][FIFO_READ] & (~ 0xFF));
					print_mutex.unlock();
					pass = false;
					break;
				}
			}
		}
	}
	
	if (pass) {
		print_mutex.lock();
		printf("fifo test %d passed.\n", n) ;
		print_mutex.unlock();
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

	FIFO::writes = (int *) calloc(NUM_FIFO * FIFO::NUM_PACKETS * DW_PER_FSB, sizeof(int));
	for (int fifo = 0; fifo < NUM_FIFO; fifo++) {
		for (int pkt = 0; pkt < FIFO::NUM_PACKETS; pkt++) {
			for (int dw = 0; dw < DW_PER_FSB; dw++) {
				FIFO::get_random(FIFO::writes + (NUM_FIFO * FIFO::NUM_PACKETS * DW_PER_FSB) + (pkt * FIFO::NUM_PACKETS) + dw);
			}
		}
	}
	
	FIFO::reads = (int *) calloc(NUM_FIFO * FIFO::NUM_PACKETS * DW_PER_FSB, sizeof(int));


	/* clear fifo interrupts */
	for (int fifo = 0; fifo < NUM_FIFO; fifo++)
		clear_int(fifo);

	/* spawn user threads */
	vector<thread> users;
	for (int i = 0; i < FIFO::NUM_USERS; i++) {
		users.push_back(thread(user, i));
	}
	for (int i = 0; i < FIFO::NUM_USERS; i++) {
		users[i].join();
	} 	


	/* what has been written */
//	for (int fifo = 0; fifo < NUM_FIFO; fifo++) {
//		for (int pkt = 0; pkt < FIFO::NUM_PACKETS; pkt++) {
//			cout << "fifo " << fifo << ", packet " << pkt << ": "; 
//			for (int dw = 0; dw < DW_PER_FSB; dw++)
//				cout << FIFO::writes[(fifo * FIFO::NUM_PACKETS * DW_PER_FSB) +  (pkt * DW_PER_FSB) + dw] << " ";
//		}
//		cout << endl;
//	}

	free(FIFO::writes);
	free(FIFO::reads);
	return 0;
}


