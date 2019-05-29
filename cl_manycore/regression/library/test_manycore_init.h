#pragma once
#ifdef COSIM
#include <bsg_manycore.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_mmio.h>
#else
#include "bsg_manycore.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore_mmio.h"
#endif
#include <inttypes.h>
#include "../cl_manycore_regression.h"

