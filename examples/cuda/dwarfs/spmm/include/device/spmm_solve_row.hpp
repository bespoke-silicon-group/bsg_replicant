#pragma once
#include "sparse_matrix.h"
#include "spmm.hpp"

void spmm_solve_row(int Ai);
void spmm_scalar_row_product(float Aij, int Bi);

