#include <stdio.h>
#ifndef COSIM
	#include <bsg_manycore_print.h>
#else
	#include "bsg_manycore_print.h"
#endif

void hb_mc_print_hex (uint8_t *p) {
	for (int i = 0; i < 16; i++) {
		printf("%x ", (p[15-i] & 0xFF));
	}
	printf("\n");
}

