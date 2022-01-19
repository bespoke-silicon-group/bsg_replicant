ifndef SPMM_RV
# use dynamic work scheduling
SPMM_RV				= spmm_dynamic.riscv.rvo
endif
ifndef SPMM_SOLVE_ROW_RV
# use hash table to solve row
SPMM_SOLVE_ROW_RV		= spmm_solve_row_hash_table
SPMM_HASH_TABLE			= yes
endif
ifndef SPMM_HASH_TABLE
# use the hash table common code
SPMM_HASH_TABLE			= yes
endif
ifndef SPMM_COMPUTE_OFFSETS_RV
# use sum tree to compute offsets
SPMM_COMPUTE_OFFSETS_RV		= spmm_compute_offsets_sum_tree
endif
ifndef SPMM_COPY_RESULTS_RV
# use copy results
SPMM_COPY_RESULTS_RV		= spmm_copy_results
endif
ifndef SPMM_SORT_ROW_RV
# use sort
SPMM_SORT_ROW_RV		= spmm_sort_row_stdsort
endif
ifndef SPMM_SOLVE_ROW_LOCAL_DATA_WORDS
# use local memory
SPMM_SOLVE_ROW_LOCAL_DATA_WORDS	= $(shell echo 256*3|bc)
endif
ifndef SPMM_PREFETCH
# use prefetch
SPMM_PREFETCH                   = yes
endif
ifndef SPMM_PREFETCH_N
# prefetch factor of 4
SPMM_PREFETCH_N			= 4
endif
ifndef SPMM_ALIGNED_TABLE
# use an aligned hash table
SPMM_ALIGNED_TABLE		= yes
endif
ifndef SPMM_SKIP_SORTING
# do sort results
SPMM_SKIP_SORTING		= yes
endif
ifndef SPMM_WORK_GRANULARITY
# tiles grab four work items at once
SPMM_WORK_GRANULARITY		= 4
endif
