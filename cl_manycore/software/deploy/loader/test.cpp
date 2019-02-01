#include "spmd_loader.h"
#include <map>
#include <iostream>
#include <vector>
using namespace std;


static char *filename = "/home/ahari/bsg_manycore/software/spmd/bsg_dram_loopback_cache/main.riscv";

int main () {
	parse_elf(filename);
	for (int i = 0; i < 10; i++)
		print_hex(text_pkts[i]);
	printf("data packet: ");
	print_hex(data_pkts[0]);
	return 0;
}
