#pragma once
#ifndef __PR_PULL_BENCHMARK_HPP
#define __PR_PULL_BENCHMARK_HPP

#include "hb_intrinsics.h"
#include "infra_hb/host/arg_parser.hpp"
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream> 
#include <atomic>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_cuda.h>
#include <cl_manycore_regression.h>


using hammerblade::Device;
using hammerblade::Vector;
using hammerblade::GraphHB;
using hammerblade::GlobalScalar;

#endif
