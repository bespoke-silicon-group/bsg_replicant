.text
.globl test_mutex_asm
test_mutex_asm:
        // a0 = mtx : bsg_mcs_mutex* in dram
        // a1 = lcl : bsg_mcs_mutex_node* in dmem
        // a2 = lcl_as_glbl : bsg_mcs_mutex* in dmem, global pointer
        li      s1, ITERS
        mv      s2, ra
test_mutex_asm_loop:   
        call    noncritical_region
        call    bsg_mcs_mutex_acquire
        call    critical_region
        addi    s1, s1, -1        
        call    bsg_mcs_mutex_release
        bnez    s1, test_mutex_asm_loop
        mv      ra, s2
        ret

        .globl test_spin_lock_asm
test_spin_lock_asm:
        // a0 = spin lock location : int*
        li      s1, ITERS
        mv      s2, ra
test_spin_lock_asm_loop:   
        call    noncritical_region
        call    spin_acquire
        call    critical_region
        addi    s1, s1, -1
        call    spin_release
        bnez    s1, test_spin_lock_asm_loop
        mv      ra, s2
        ret
#define MAX_WAIT_TIME 1024
spin_acquire:
        // a0 = spin lock location : int*
        li      t2, 1 // back off counter
        li      t0, 1 // always 1
        li      t3, MAX_WAIT_TIME
spin_acquire_loop:      
        amoswap.w.aq t1, t0, 0(a0)
        beqz    t1, spin_acquire_ret
        mv      t1, t2
spin_acquire_exp_backoff_loop:
        addi    t1, t1, -1
        bnez    t1, spin_acquire_exp_backoff_loop
        // set next wait time
        slli    t2, t2, 1
        bgt     t3, t2, spin_acquire_loop
        mv      t2, t3
        j       spin_acquire_loop
spin_acquire_ret:       
        ret

spin_release:
        // a0 = spin lock location : int*
        amoswap.w.rl   zero, zero, 0(a0)
        ret
