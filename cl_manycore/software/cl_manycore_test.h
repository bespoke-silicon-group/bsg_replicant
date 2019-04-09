#ifndef READ_WRITE_TEST_H
#define READ_WRITE_TEST_H

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

#ifdef COSIM
	#include <utils/sh_dpi_tasks.h>
	#include "fpga_pci_sv.h"
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_mem.h"
	#include "bsg_manycore_loader.h"
	#include "bsg_manycore_errno.h"	
#else
	#include <bsg_manycore_driver.h>
	#include <bsg_manycore_mem.h>
	#include <bsg_manycore_loader.h>
	#include <bsg_manycore_errno.h>
#endif

#include "../libraries/bsg_manycore_errno.h"


#define return_code(CONDITION, LABEL, ...) \
	do {                        \
		if (CONDITION) {          \
			goto LABEL;             \
		}                         \
	} while (0)

#endif

