#pragma once
#include <algorithm>
#include "spmm.hpp"

static void spmm_sort_row(int Ai)
{
    spmm_partial_t *partials = reinterpret_cast<spmm_partial_t*>(A_lcl.alg_priv_ptr[Ai]);
    int n = A_lcl.mjr_nnz_ptr[Ai];
    std::sort(partials, partials+n, [](const spmm_partial_t&l, const spmm_partial_t&r) {
            return l.idx < r.idx;
        });
}
