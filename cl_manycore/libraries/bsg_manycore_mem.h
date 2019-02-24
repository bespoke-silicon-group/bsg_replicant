#ifndef BSG_MANYCORE_MEM_H
#define BSG_MANYCORE_MEM_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif

#include <stdbool.h>
#include <stdint.h>

bool hb_mc_copy_from_epa (uint8_t fd, uint32_t **buf, uint32_t x, uint32_t y, uint32_t epa, uint32_t size); 
bool hb_mc_copy_to_epa (uint8_t fd, uint32_t x, uint32_t y, uint32_t epa, uint32_t *buf, uint32_t size);

#endif
