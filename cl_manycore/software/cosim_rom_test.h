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
#include "bsg_manycore_print.h"
#include "bsg_manycore_errno.h"

#include "rom_gen.h"

void print_rom(uint8_t fd, int idx, int num) {
    uint32_t **buf = (uint32_t **) calloc(num, sizeof(uint32_t *));
    int read = hb_mc_copy_from_epa(fd, buf, 0, 0, idx, num);
    if (read == HB_MC_SUCCESS) {
        for (int i=0; i<num; i++) {
            printf("read rom data @ address %d: ", idx + i);
            hb_mc_print_data((uint8_t *) buf[i]);
        }
    }
    else {
        printf("read ROM failed.\n");
    }
    return;
}


void cosim_rom_test () {
    printf("Runing the Cosimulation: rom test\n");
    uint8_t fd = 0;

    printf("Readback manycore link monitor register\n");
    uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
    printf("recv vacancy is: %x\n", recv_vacancy);

    printf("Readback ROM from tile (%d, %d)\n", 0, 0);
    print_rom(fd, 0, ROM_DEPTH);

    return;
}

#endif