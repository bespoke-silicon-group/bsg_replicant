#ifndef HOST_H
#define HOST_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/time.h>
#include <unistd.h>

// #define COSIM 

#ifdef COSIM
#include "fpga_pci_sv.h"
#endif

struct Host {
	char *buf, *buf_cpy;
	uint32_t head, buf_size;
	uint32_t (*get_tail) ();
	void (*write_cfg_reg) (struct Host *host, uint32_t val);
	void (*write_wr_addr_high) (struct Host *host, uint32_t val);
	void (*write_wr_addr_low) (struct Host *host, uint32_t val);
	void (*write_wr_head) (struct Host *host, uint32_t val);
	void (*write_wr_len) (struct Host *host, uint32_t val);
	void (*write_wr_buf_size) (struct Host *host, uint32_t val);
	void (*start_write) (struct Host *host);
	void (*stop) (struct Host *host);
	bool (*pop) (struct Host *host, uint32_t pop_size);
	void (*print) (struct Host *host, uint32_t ofs, uint32_t size); 
	uint32_t (*get_pkt_num) (struct Host *host);
};

/* These functions can't be used with deployed programs. */
uint32_t get_wr_addr_high (struct Host * host) {
	uint32_t high = (uint32_t) ((((uint64_t) host->buf) & 0xffffffff00000000) >> 32);
	return high;
}

uint32_t get_wr_addr_low (struct Host * host) {
	uint32_t low = (uint32_t) (((unsigned long) host->buf) & 0x00000000ffffffff);
	return low;
}

/* --------------------------------------------------------------------- */
/* Tests */

void check_mem (struct Host *host, int num_pages) {
	
	int fsb;
	bool pass = true;
	uint32_t counter = 0x0;
	
	int end = ((num_pages * 4096) >  (host->buf_size - 64)) ? (host->buf_size - 64) : (num_pages * 4096);
	for (fsb = 2*16; fsb < (end/10); fsb += 16) {
		for (int i = 0; i < 8; i++) { // data bytes
			uint8_t byte = host->buf[fsb + i];
			uint8_t id = (byte & 0xE0) >> 5; 
			uint8_t data = byte & (0x1F);			
			if (id != i || data != counter) {
				printf("check_mem(): mismatch @ fsb #%d\n", fsb);
				printf("id: %d, data: %d\n", id, data);
				printf("expected id: %d, expected data: %d\n", i, counter);
				pass = false;
				//goto test_finish;
			} 
		}		
		for (int i = 8; i < 10; i++) {
			// check header
		}
		counter = (counter + 1) % 32;
	}	
	test_finish:
	if (pass) 
		printf("Pass! Memory is correct.\n");
	else
		printf("Fail!\n");
}

void print_pop(struct Host *host, uint32_t pop_size) {
	int temp = host->head - pop_size;
	int start = (temp >= 0) ? temp : host->buf_size + temp; 

	int end = (start + pop_size < host->buf_size) ? (start + pop_size) : host->buf_size;
	for (uint32_t i = start; i < end; i++) {
		printf("0x%2X", 0xFF & host->buf_cpy[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
		else
			printf(" ");
	}

	if (host->buf_size - start < pop_size) {
		for (uint32_t i = 0; i < host->head; i++) {
			printf("0x%2X", 0xFF & host->buf_cpy[i]);
			if ((i + 1) % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}
	}
}

/* User program continually pops 64B followed by 128B i
 * Bug to be fixed: Printed pop data is wrong even though memory is correct
 * according to check_mem().
 * TODO: only works in cosim because of sv_pause. 
 * */

void pop_loop (struct Host *host) {
	int pts=5;
	double init_time[pts];
	uint32_t init_pkt_num[pts];
	int loop_num = 1000000;
	uint32_t read_byte = 4096;
	struct timespec cur_time;
	double time_us;	
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &cur_time);
	double start_time = (double)(cur_time.tv_sec*1000000 + cur_time.tv_nsec/1000);
	uint32_t host_pkt_num;
	for (int i=0; i<loop_num; i++) {
//	while (true) {
//		bool read_64 = host->pop(host, 64);
//		if (read_64) {
//			print_pop(host, 64);
//			printf("\n\n");
//		}
//		bool read_128 = host->pop(host, 128);
//		if (read_128) {
//			print_pop(host, 128);
//			printf("\n\n");
//		}	
//		if (!read_64) {
//			printf("Fail. User could not read 64B. The test is stuck. \n.");
//		}
		bool read_trunk = host->pop(host, read_byte);
                if (read_trunk) {
	        	printf(".", read_byte);
		}
		if (i==0) {
		 	host_pkt_num = host->get_pkt_num(host); 
        		clock_gettime(_POSIX_MONOTONIC_CLOCK, &cur_time);
        		time_us = (double)(cur_time.tv_sec*1000000 + cur_time.tv_nsec/1000);
        		printf("\nCL has sent %u packets @ %f us.\n", host_pkt_num, time_us);

			init_time[0] = time_us;
			init_pkt_num[0] = host_pkt_num;
		} 
		else if (i==(int)(loop_num/(pts-1))) {
		 	host_pkt_num = host->get_pkt_num(host); 
        		clock_gettime(_POSIX_MONOTONIC_CLOCK, &cur_time);                   	
        		time_us = (double)(cur_time.tv_sec*1000000 + cur_time.tv_nsec/1000);
        		printf("\nCL has sent %u packets @ %f us.\n", host_pkt_num, time_us);
			init_time[1] = time_us;
			init_pkt_num[1] = host_pkt_num;
		}
		else if (i==(int)(loop_num/(pts-1)*2-1)) {
		 	host_pkt_num = host->get_pkt_num(host); 
        		clock_gettime(_POSIX_MONOTONIC_CLOCK, &cur_time);
        		time_us = (double)(cur_time.tv_sec*1000000 + cur_time.tv_nsec/1000);
        		printf("\nCL has sent %u packets @ %f us.\n", host_pkt_num, time_us);
			init_time[2] = time_us;
			init_pkt_num[2] = host_pkt_num;	
		}
		else if (i==(int)(loop_num/(pts-1)*3-1)) {
		 	host_pkt_num = host->get_pkt_num(host); 
        		clock_gettime(_POSIX_MONOTONIC_CLOCK, &cur_time);
        		time_us = (double)(cur_time.tv_sec*1000000 + cur_time.tv_nsec/1000);
        		printf("\nCL has sent %u packets @ %f us.\n", host_pkt_num, time_us);

			init_time[3] = time_us;
			init_pkt_num[3] = host_pkt_num;
		}
		else if (i==(int)(loop_num/(pts-1)*4-1)) {
		 	host_pkt_num = host->get_pkt_num(host); 
        		clock_gettime(_POSIX_MONOTONIC_CLOCK, &cur_time);
        		time_us = (double)(cur_time.tv_sec*1000000 + cur_time.tv_nsec/1000);
        		printf("\nCL has sent %u packets @ %f us.\n", host_pkt_num, time_us);

			init_time[4] = time_us;
			init_pkt_num[4] = host_pkt_num;

			printf("===> initial time: %f, end time %f; %u, %u\n", init_time[0], init_time[pts-1], init_pkt_num[0], init_pkt_num[pts-1]); 

		double BW0 = ((init_pkt_num[1]-init_pkt_num[0])/(init_time[1]-init_time[0]))*512*1000000;
		double BW1 = ((init_pkt_num[2]-init_pkt_num[1])/(init_time[2]-init_time[1]))*512*1000000;
		double BW2 = ((init_pkt_num[3]-init_pkt_num[2])/(init_time[3]-init_time[2]))*512*1000000;
		double BW3 = ((init_pkt_num[4]-init_pkt_num[3])/(init_time[4]-init_time[3]))*512*1000000;
		double BWavg = (BW0 + BW1 + BW2 + BW3)/4;	
		printf("Start Writting at %f, Bandwidth is %6.3f, %6.3f, %6.3f, %6.3f, %f(average); Latency is %f.\n", start_time, BW0, BW1, BW2, BW3, BWavg, init_time[4] - init_pkt_num[4]/BWavg/512/1000000 - start_time);
		}	
		#ifdef COSIM
		sv_pause(1);
		#else
		//sleep(0.9);
		#endif 
	}
}

/*
 * works for cosim only. need to change this. 
 * */
void clear_buf(struct Host *host) {
	for (int i = 0; i < host->buf_size + 64; i++) {
		host->buf[0] = 0;
	}
}

void empty_full (struct Host *host) {
	int i;
	if (host->buf_size > 4096) {
		printf("Test not run because buffer is bigger than 4096B.\n");
		return;
	}
	for (i = 0; i < 10; i++) {
		clear_buf(host);
		host->write_wr_head(host, 0);	
		host->head = 0;
		// fill the buffer
		host->start_write(host);
		#ifdef COSIM
		sv_pause(10); // fill buffer
		#else
		sleep(1);
		#endif

		/* check buffer is full */
		uint32_t tail = *((uint32_t *) (host->buf + host->buf_size));
 		
		if (tail != host->buf_size - 64) {
			printf("Fail. Buffer not full; Tail is at: %u\n", tail);
			return;
		}

		/* check tail is stationary */
		bool pass = true;
		for (i = 0; i < 3; i++) {
			uint32_t new_tail = *((uint32_t *) (host->buf + host->buf_size));
			if (new_tail != tail) {
				pass = false;
				break;
			}
			tail = new_tail;
		}
		if (!pass) {
			printf("Fail. Tail has moved in spite of the fact that the buffer is full.\n");
			return;
		}	
		/* stop data generation */
		host->stop(host);
		/* pop all data from buffer */	
		for (int i = 0; i < host->buf_size / 64; i++) {
			uint32_t new_tail = *((uint32_t *) (host->buf + host->buf_size));
			if (new_tail != tail) {
				printf("Fail. Device wrote data after host disabled writes.\n");
				return;
			}
			host->pop(host, 64);
		}
		/* check that host emptied the buffer */
		if (host->head != tail)
			printf("Fail. Host did not read all of the data from the buffer.\n");
		
		/* try reading from empty buffer */
		host->pop(host, 64);
		if (host->head != tail) {
			printf("Fail. Host read from an empty buffer.\n");
			return;
		}
					
		printf("Test #%d passes\n", i+1);
	}
	printf("empty_full: All tests pass!\n");
}

#endif
