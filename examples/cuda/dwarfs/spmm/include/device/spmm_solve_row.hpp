#pragma once
#include "sparse_matrix.h"
#include "spmm.hpp"


#ifndef SPMM_SOLVE_ROW_LOCAL_DATA_WORDS
#define SPMM_SOLVE_ROW_LOCAL_DATA_WORDS         \
    (7*128)
#endif

void spmm_solve_row_init();
void spmm_solve_row(int Ai);


