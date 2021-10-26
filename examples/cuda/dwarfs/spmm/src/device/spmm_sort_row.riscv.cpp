#include "spmm_sort_row.hpp"
#include <algorithm>
#ifndef SPMM_SKIP_SORTING
void spmm_sort_row(int Ci)
{
    spmm_partial_t *partials = reinterpret_cast<spmm_partial_t*>(C_lcl.alg_priv_ptr[Ci]);
    int n = C_lcl.mjr_nnz_ptr[Ci];
    std::sort(partials, partials+n, [](const spmm_partial_t&l, const spmm_partial_t&r) {
            return l.idx < r.idx;
        });
}
#endif
