#ifndef COSIM_ROM_TEST_H
#define COSIM_ROM_TEST_H

#ifndef _BSD_SOURCE
    #define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE 500
#endif

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "bsg_manycore_driver.h"
#include "bsg_manycore_mem.h"
#include "bsg_manycore_errno.h"
#include "bsg_manycore_packet.h"

int print_rom(uint8_t fd, int idx, int num) {
    hb_mc_request_packet_t buf[num];
    int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 0, idx, num);
    if (read == HB_MC_SUCCESS) {
        for (int i=0; i<num; i++) {
            printf("read rom data @ address %d: ", idx + i);
            uint8_t *pkt = (uint8_t *) &buf[i];
            for (int i=6; i<6+4; i++) {
              printf("%02x ", (pkt[15-i] & 0xFF));
            }
            printf("\n");
        }
        return 1;
    }
    else {
        printf("read from ROM failed.\n");
        return 0;
    }
}


int cosim_rom_test () {
    printf("Runing the Cosimulation: rom test\n");
    uint8_t fd = 0;

    printf("Readback manycore link monitor register\n");
    uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
    printf("recv vacancy is: %x\n", recv_vacancy);

    printf("Readback ROM from tile (%d, %d)\n", 0, 0);
    int success = print_rom(fd, 0, 12);

    return success;
}

#endif

