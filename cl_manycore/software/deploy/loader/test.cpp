#include "spmd_loader.h"
#include <map>
#include <iostream>
#include <vector>
using namespace std;

static string filename = "/home/ahari/bsg_manycore/software/spmd/hello/main.riscv";

void print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) 
		cout << hex << (int) (p[i] & 0xFF) << " ";
	cout << endl;
}
	
int main () {
	uint8_t **packets = init_icache();
	print_hex(packets[0]);
	


//	map<std::string, uint64_t> _map= load_elf(filename.c_str());	
//	for(map<std::string, uint64_t>::iterator it = _map.begin(); it != _map.end(); it++) {
//		cout << "String: " << it->first << ", Number: " << it->second << endl;
//	}
		
//	vector<vector<vector<char *>>> packets = get_packets(filename);
//	cout << "# segments: " << packets.size() << ", # tiles: " << packets[0].size() << ", # packets for 1st segment: " << packets[0][0].size() <<  endl;
//	for (int segment = 0; segment < packets.size(); segment++) {
//		int num_row = 0, num_col = 0;
//		if (segment == TEXT) {
//			cout << "Text Segment" << endl;
//			num_row = NUM_X; 
//			num_col = NUM_Y;
//		}
//		else if (segment == DATA) {
//			cout << "Data Segment" << endl;
//			num_row = 1;
//			num_col = 1;
//		}
//		else {
//			cout << "Segment " << segment << " of unknown type." << endl; 
//			continue;
//		}
//
//
//		for (int row = 0; row < num_row; row++) {
//			for (int col = 0; col < num_col; col++) {
//				if (row == 0 && col == 0) {
//					cout << "(X, Y) = (" << col << ", " << row << ")" << endl;
//					for (int packet = 0; packet < packets[segment][row*NUM_X + col].size(); packet++) {
//						char *p = packets[segment][row*NUM_X + col][packet]; 
//						cout << "Packet: "; 
//						print_hex(packets[segment][row*NUM_X + col][packet]); 
//						cout << endl;
//					}
//				}
//			}
//		}
//		cout << endl << endl;
//	}
	return 0;
}
