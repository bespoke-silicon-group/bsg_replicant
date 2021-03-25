/*
 * This kernel prints the Hello World message 
 */

// BSG_TILE_GROUP_X_DIM and BSG_TILE_GROUP_Y_DIM must be defined
// before bsg_manycore.h and bsg_tile_group_barrier.h are
// included. bsg_tiles_X and bsg_tiles_Y must also be defined for
// legacy reasons, but they are deprecated.
#define BSG_TILE_GROUP_X_DIM 1
#define BSG_TILE_GROUP_Y_DIM 1
#define bsg_tiles_X BSG_TILE_GROUP_X_DIM
#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM
#include <bsg_manycore.h>
#include <bsg_tile_group_barrier.h>

/* We wrap all external-facing C++ kernels with `extern "C"` to
 * prevent name mangling 
 */
extern "C" {
    int  __attribute__ ((noinline)) kernel_beq_t(int arg) {
        asm volatile(
          "li t0, 0xb08aa953;"
          "li t1, 0x1a170747;"
          "li t2, 0x0f05af9f;"
          "li a0, 0x0e514f85;"
          "li a1, 0x9b8392ae;"
          "li a2, 0x200b4645;"
          "li a3, 0x03eec17c;"
          "li a4, 0x9eff0bd2;"
          "li a5, 0xe159869a;"
          "li a3, 0xc06cba73;"
          "li a7, 0x2af2f60a;"
          "li s2, 0xeab53432;"
          "li s3, 0x7ea683e1;"
          "li s4, 0xfc472a2e;"
          "li s5, 0x87e5957f;"
          "li s6, 0x112b3588;"
          "li s7, 0x08defccf;"
          "li s8, 0x4b2252b6;"
          "li s9, 0xf43ad881;"
          "li s10, 0xe947face;"
          "li s11, 0x9ba8db96;"
          "li t3, 0xc0ce6543;"
          "li t4, 0xaa4b8981;"
          "li t5, 0x21011a73;"
          "li t6, 0x0198e2f3;"

                       );
        asm volatile ("addi zero,zero,1;");
        asm volatile (
          "m0: beq t0, t0, m99;"
          "m1: beq t1, t1, m98;"
          "m2: beq t2, t2, m97;"
          "m3: beq a0, a0, m96;"
          "m4: beq a1, a1, m95;"
          "m5: beq a2, a2, m94;"
          "m6: beq a3, a3, m93;"
          "m7: beq a4, a4, m92;"
          "m8: beq a5, a5, m91;"
          "m9: beq a3, a3, m90;"
          "m10: beq a7, a7, m89;"
          "m11: beq s2, s2, m88;"
          "m12: beq s3, s3, m87;"
          "m13: beq s4, s4, m86;"
          "m14: beq s5, s5, m85;"
          "m15: beq s6, s6, m84;"
          "m16: beq s7, s7, m83;"
          "m17: beq s8, s8, m82;"
          "m18: beq s9, s9, m81;"
          "m19: beq s10, s10, m80;"
          "m20: beq s11, s11, m79;"
          "m21: beq t3, t3, m78;"
          "m22: beq t4, t4, m77;"
          "m23: beq t5, t5, m76;"
          "m24: beq t6, t6, m75;"
          "m25: beq t0, t0, m74;"
          "m26: beq t1, t1, m73;"
          "m27: beq t2, t2, m72;"
          "m28: beq a0, a0, m71;"
          "m29: beq a1, a1, m70;"
          "m30: beq a2, a2, m69;"
          "m31: beq a3, a3, m68;"
          "m32: beq a4, a4, m67;"
          "m33: beq a5, a5, m66;"
          "m34: beq a3, a3, m65;"
          "m35: beq a7, a7, m64;"
          "m36: beq s2, s2, m63;"
          "m37: beq s3, s3, m62;"
          "m38: beq s4, s4, m61;"
          "m39: beq s5, s5, m60;"
          "m40: beq s6, s6, m59;"
          "m41: beq s7, s7, m58;"
          "m42: beq s8, s8, m57;"
          "m43: beq s9, s9, m56;"
          "m44: beq s10, s10, m55;"
          "m45: beq s11, s11, m54;"
          "m46: beq t3, t3, m53;"
          "m47: beq t4, t4, m52;"
          "m48: beq t5, t5, m51;"
          "m49: beq t6, t6, m50;"
          "m50: beq t0, t0, ms;"
          "m51: beq t1, t1, m49;"
          "m52: beq t2, t2, m48;"
          "m53: beq a0, a0, m47;"
          "m54: beq a1, a1, m46;"
          "m55: beq a2, a2, m45;"
          "m56: beq a3, a3, m44;"
          "m57: beq a4, a4, m43;"
          "m58: beq a5, a5, m42;"
          "m59: beq a3, a3, m41;"
          "m60: beq a7, a7, m40;"
          "m61: beq s2, s2, m39;"
          "m62: beq s3, s3, m38;"
          "m63: beq s4, s4, m37;"
          "m64: beq s5, s5, m36;"
          "m65: beq s6, s6, m35;"
          "m66: beq s7, s7, m34;"
          "m67: beq s8, s8, m33;"
          "m68: beq s9, s9, m32;"
          "m69: beq s10, s10, m31;"
          "m70: beq s11, s11, m30;"
          "m71: beq t3, t3, m29;"
          "m72: beq t4, t4, m28;"
          "m73: beq t5, t5, m27;"
          "m74: beq t6, t6, m26;"
          "m75: beq t0, t0, m25;"
          "m76: beq t1, t1, m24;"
          "m77: beq t2, t2, m23;"
          "m78: beq a0, a0, m22;"
          "m79: beq a1, a1, m21;"
          "m80: beq a2, a2, m20;"
          "m81: beq a3, a3, m19;"
          "m82: beq a4, a4, m18;"
          "m83: beq a5, a5, m17;"
          "m84: beq a3, a3, m16;"
          "m85: beq a7, a7, m15;"
          "m86: beq s2, s2, m14;"
          "m87: beq s3, s3, m13;"
          "m88: beq s4, s4, m12;"
          "m89: beq s5, s5, m11;"
          "m90: beq s6, s6, m10;"
          "m91: beq s7, s7, m9;"
          "m92: beq s8, s8, m8;"
          "m93: beq s9, s9, m7;"
          "m94: beq s10, s10, m6;"
          "m95: beq s11, s11, m5;"
          "m96: beq t3, t3, m4;"
          "m97: beq t4, t4, m3;"
          "m98: beq t5, t5, m2;"
          "m99: beq t6, t6, m1;"

                     );
        asm volatile (" ms: addi zero,zero,2");
        return 0;
    }
}
