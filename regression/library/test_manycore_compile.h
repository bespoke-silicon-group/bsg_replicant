#ifndef TEST_MANYCORE_COMPILE_H
#define TEST_MANYCORE_COMPILE_H
#ifdef COSIM
#include <bsg_manycore.h>
#include <bsg_manycore_mmio.h>
#else
#include "bsg_manycore.h"
#include "bsg_manycore_mmio.h"
#endif
#include <inttypes.h>
#include "../cl_manycore_regression.h"

#endif
