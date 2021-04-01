#ifndef __SSSP_BENCHMARK_HPP
#define __SSSP_BENCHMARK_HPP

#pragma once
#include "hb_intrinsics.h"
#include "infra_hb/host/arg_parser.hpp"
#include "infra_hb/host/priority_queue.hpp"
#include <string.h>
#include <stdio.h>
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
using hammerblade::WGraphHB;
using hammerblade::GlobalScalar;
using hammerblade::BucketPriorityQueue;
using hammerblade::Bucket;
#endif
