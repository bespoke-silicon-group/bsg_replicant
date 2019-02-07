#include "spmd_loader.h"
#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;


string fname = "/home/centos/src/project_data/bsg_f1/cl_manycore/software/deploy/loader/icache_gen.txt"; 
ofstream myfile;

void dump_file (uint8_t *p) {
	for (int i = 0; i < 16; i++) 
		myfile << hex << (int) (p[i] & 0xFF) << " ";
	myfile << "\n";
}

int main () {

	myfile.open(fname);
	
	uint8_t **icache_pkts = init_icache();

	for (int i = 0; i < NUM_ICACHE; i++) {
		dump_file(icache_pkts[i]);
	}

//	uint8_t *correct = get_pkt_12x12(1 << (ADDR_BIT - 1), 0, 0, 13);	
//	print_hex(correct);


//	printf("text pkts: \n");
//	for (int i = 0; i < 10; i++)
//		print_hex(text_pkts[i]);
//	printf("data packets: ");
//	print_hex(data_pkts[0]);
	return 0;
}
