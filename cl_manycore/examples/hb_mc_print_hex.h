#ifndef HB_MC_PRINT_HEX_H
#define HB_MC_PRINT_HEX_H
#include <stdio.h>
#include <stdint.h>
static void hb_mc_print_hex(uint8_t *bytes)
{
    for (int i = 0; i < 16; i++) {
	printf("%02x ", bytes[15-i]);
    }
    printf("\n");
}

#endif
