#ifndef __LIBRARY_TESTS_H
#define __LIBRARY_TESTS_H

#ifdef COSIM

#include <utils/sh_dpi_tasks.h>
#include "fpga_pci_sv.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore_mem.h"
#include "bsg_manycore_loader.h"
#include "bsg_manycore_errno.h"	
#include "bsg_manycore_printing.h"

#else // !COSIM

#include <bsg_manycore_driver.h>
#include <bsg_manycore_mem.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_printing.h>

#endif // #ifdef COSIM

#include "../cl_manycore_regression.h"

#endif // __LIBRARY_TESTS_H
