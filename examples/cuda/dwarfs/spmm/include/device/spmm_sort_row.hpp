#pragma once
#include "spmm.hpp"

#ifdef SPMM_SKIP_SORTING
static void spmm_sort_row(int Ci)
{
}
#else
void spmm_sort_row(int Ci);
#endif
