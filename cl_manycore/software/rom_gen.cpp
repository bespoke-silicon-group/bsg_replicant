// this program generate the rom with manycore configuration information.

#include <iostream>
#include <fstream>
#include <string>

#include "rom_gen.h"

using namespace std;

class RomWriter {
  public:
  static string convert_to_binary_string(int, int);
  static string convert_to_hex_string(int, int);
};

string RomWriter::convert_to_binary_string (int data, int width) {
  string rom_binary = "";
  for (int i=0; i<width; i++) {
    int r2l_idx = width - 1 - i;
    bool bit_val = (data & (1 << r2l_idx)) >> r2l_idx;
    rom_binary += bit_val ? "1" : "0";
  }
  return rom_binary;
}

string RomWriter::convert_to_hex_string (int data, int width) {
  string rom_hex = "";
  for (int i=0; i<width/8; i++) {
    int r2l_idx = width - 1 - 8*i;
    uint8_t byte_val = (data & (0xFF << r2l_idx)) >> r2l_idx;
    char hex_chars[2];
    sprintf(hex_chars, "%02x", byte_val);
    rom_hex += (string) hex_chars;
  }
  return rom_hex;
}

int main() {

  RomWriter writer;

  ofstream rom_file ("bsg_bladerunner.rom");
  if (rom_file.is_open())
  {
    rom_file << "@0\n";
    for (int i=0; i<ROM_DEPTH; i++) {
      string binary_string = writer.convert_to_binary_string(ROM_DATA[i], ROM_WIDTH);
      rom_file << binary_string << endl;
      // print the results:
      string hex_string = writer.convert_to_hex_string(ROM_DATA[i], ROM_WIDTH);
      printf("%-20s :", ROM_KEY[i]);
      printf("%s \n", hex_string.c_str());
    }
    rom_file.close();
  }
  else cout << "Unable to open file\n";
  return 0;
}
