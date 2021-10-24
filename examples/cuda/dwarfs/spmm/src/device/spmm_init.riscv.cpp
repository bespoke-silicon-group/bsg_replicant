#include "spmm.hpp"
#include "spmm_barrier.hpp"

thread std::atomic<intptr_t> *spmm_mem_pool = nullptr;
thread sparse_matrix_t A_lcl;
thread sparse_matrix_t B_lcl;
thread sparse_matrix_t C_lcl;

thread sparse_matrix_t *A_glbl_p;
thread sparse_matrix_t *B_glbl_p;
thread sparse_matrix_t *C_glbl_p;

__attribute__((section(".dram")))
bsg_mcs_mutex_t mtx;

void spmm_init(sparse_matrix_t *__restrict__ A_ptr, // csr
               sparse_matrix_t *__restrict__ B_ptr, // csr
               sparse_matrix_t *__restrict__ C_ptr, // csr
               std::atomic<intptr_t> *mem_pool_arg) // mem pool
{
    A_lcl = *A_ptr;
    B_lcl = *B_ptr;
    C_lcl = *C_ptr;

    A_glbl_p = A_ptr;
    B_glbl_p = B_ptr;
    C_glbl_p = C_ptr;
    
    spmm_mem_pool = mem_pool_arg;
    barrier::spmm_barrier_init();
    
}
