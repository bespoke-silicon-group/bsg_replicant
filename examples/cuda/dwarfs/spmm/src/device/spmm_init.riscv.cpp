#include "spmm.hpp"
#include "spmm_barrier.hpp"

thread std::atomic<intptr_t> *spmm_mem_pool = nullptr;
thread sparse_matrix_partition_t A_part_lcl;
thread sparse_matrix_partition_t B_part_lcl;
thread sparse_matrix_partition_t C_part_lcl;

thread sparse_matrix_partition_t *A_part_glbl_p;
thread sparse_matrix_partition_t *B_part_glbl_p;
thread sparse_matrix_partition_t *C_part_glbl_p;

__attribute__((section(".dram")))
bsg_mcs_mutex_t mtx;

void spmm_init(sparse_matrix_partition_t *__restrict__ A_ptr, // csr
               sparse_matrix_partition_t *__restrict__ B_ptr, // csr
               sparse_matrix_partition_t *__restrict__ C_ptr, // csr
               std::atomic<intptr_t> *mem_pool_arg) // mem pool
{
    A_part_lcl = *A_ptr;
    B_part_lcl = *B_ptr;
    C_part_lcl = *C_ptr;

    A_part_glbl_p = A_ptr;
    B_part_glbl_p = B_ptr;
    C_part_glbl_p = C_ptr;
    
    spmm_mem_pool = mem_pool_arg;
    barrier::spmm_barrier_init();
    
}
