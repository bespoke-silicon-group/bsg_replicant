#ifndef BITS_H
#define BITS_H

/*!
 * Helper function that gets bits of an int.
 * @param data value to get bits from. 
 * @param start starting bit. 
 * @param size number of bits to retrieve.
 * @return desired bits of data. They are right-shifted to the LSB.
 * */

uint32_t get_bits(uint32_t data, uint8_t start,  uint8_t size) {
	uint32_t mask = UINT_MAX;
	mask = mask << start;
	uint32_t bits = (data & mask) >> start;
	return bits;
}

#endif
