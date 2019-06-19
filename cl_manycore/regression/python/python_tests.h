#ifndef __PYTHON_TESTS
#define __PYTHON_TESTS

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <bsg_manycore.h>
#include "../cl_manycore_regression.h"

#define __BSG_STRINGIFY(arg) #arg
#define BSG_STRINGIFY(arg) __BSG_STRINGIFY(arg)
#endif
