#ifndef BSG_MANYCORE_DRIVER_H
#define BSG_MANYCORE_DRIVER_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif


#include <stdint.h>
#include <limits.h>



#ifndef COSIM
#include <bsg_manycore_packet.h>
#else
#include "bsg_manycore_packet.h"
#endif

#ifndef COSIM
static char *hb_mc_mmap_ocl (uint8_t fd);
#endif

int hb_mc_init_host (uint8_t *fd);
int hb_mc_check_dim (uint8_t fd);      
int hb_mc_write_fifo (uint8_t fd, uint8_t n, hb_mc_packet_t *packet);
int hb_mc_read_fifo (uint8_t fd, uint8_t n, hb_mc_packet_t *packet);
int hb_mc_clear_int (uint8_t fd, uint8_t n);
int hb_mc_get_host_credits (uint8_t fd);
int hb_mc_all_host_req_complete(uint8_t fd);
int hb_mc_get_recv_vacancy (uint8_t fd);
int hb_mc_can_read (uint8_t fd, uint32_t size);
int hb_mc_check_device (uint8_t fd);
uint8_t hb_mc_get_num_x ();
uint8_t hb_mc_get_num_y (); 
void hb_mc_format_request_packet(hb_mc_request_packet_t *packet, uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode);

static uint8_t num_dev = 0;
static char *ocl_table[8] = {(char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0, (char *) 0};

#define MAX_CREDITS 16

#endif
