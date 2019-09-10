#include <bsg_manycore_bits.h>

/* returns desired bits aligned to the LSB.
 * */
uint32_t hb_mc_get_bits (uint32_t val, uint32_t start, uint32_t size) {
	uint32_t mask = ((1 << size) - 1) << start;
	return ((val & mask) >> start); 
}
