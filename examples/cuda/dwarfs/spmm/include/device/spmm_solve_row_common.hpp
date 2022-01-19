#pragma once
#include "sparse_matrix.h"
#include "spmm.hpp"


#ifndef SPMM_SOLVE_ROW_LOCAL_DATA_WORDS
#define SPMM_SOLVE_ROW_LOCAL_DATA_WORDS         \
    (7*128)
#endif

static void spmm_solve_row_init();
static void spmm_solve_row(int Ci, int Ci_off, int Ci_nnz);
static void spmm_solve_row_exit();
