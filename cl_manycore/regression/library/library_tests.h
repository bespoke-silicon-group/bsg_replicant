#ifndef __LIBRARY_TESTS_H
#define __LIBRARY_TESTS_H

#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>	
#include <bsg_manycore_printing.h>

#ifdef COSIM
#include <utils/sh_dpi_tasks.h>
#include "fpga_pci_sv.h"
#else // !COSIM
#endif // #ifdef COSIM

#include "../cl_manycore_regression.h"

#endif // __LIBRARY_TESTS_H
