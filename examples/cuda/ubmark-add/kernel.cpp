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
    int  __attribute__ ((noinline)) kernel_add(int arg) {
        asm volatile(
          "li t0, 0x15cc90ac;"
          "li t1, 0x01339313;"
          "li t2, 0x5a346b68;"
          "li a0, 0x801632e4;"
          "li a1, 0x46c7f5a1;"
          "li a2, 0xebac802b;"
          "li a3, 0x023ec3ce;"
          "li a4, 0x7b586b35;"
          "li a5, 0x27218905;"
          "li a3, 0xc2540fb4;"
          "li a7, 0x73a9366a;"
          "li s2, 0x39c5b9f3;"
          "li s3, 0x9fe11e92;"
          "li s4, 0x42568eef;"
          "li s5, 0x34f69c94;"
          "li s6, 0xb6ccdbbb;"
          "li s7, 0x0373f68d;"
          "li s8, 0xd62f0eec;"
          "li s9, 0xbf7e016a;"
          "li s10, 0xb8256600;"
          "li s11, 0x1f45ced2;"
          "li t3, 0x9f9d2912;"
          "li t4, 0xf0d8c43b;"
          "li t5, 0x8fed371c;"
          "li t6, 0x6de88dbd;"

                       );
        asm volatile ("addi zero,zero,1;");
        asm volatile (
          "m0: add t0, t1, t2;"
          "m1: add t1, t2, a0;"
          "m2: add t2, a0, a1;"
          "m3: add a0, a1, a2;"
          "m4: add a1, a2, a3;"
          "m5: add a2, a3, a4;"
          "m6: add a3, a4, a5;"
          "m7: add a4, a5, a3;"
          "m8: add a5, a3, a7;"
          "m9: add a3, a7, s2;"
          "m10: add a7, s2, s3;"
          "m11: add s2, s3, s4;"
          "m12: add s3, s4, s5;"
          "m13: add s4, s5, s6;"
          "m14: add s5, s6, s7;"
          "m15: add s6, s7, s8;"
          "m16: add s7, s8, s9;"
          "m17: add s8, s9, s10;"
          "m18: add s9, s10, s11;"
          "m19: add s10, s11, t3;"
          "m20: add s11, t3, t4;"
          "m21: add t3, t4, t5;"
          "m22: add t4, t5, t6;"
          "m23: add t5, t6, t0;"
          "m24: add t6, t0, t1;"
          "m25: add t0, t1, t2;"
          "m26: add t1, t2, a0;"
          "m27: add t2, a0, a1;"
          "m28: add a0, a1, a2;"
          "m29: add a1, a2, a3;"
          "m30: add a2, a3, a4;"
          "m31: add a3, a4, a5;"
          "m32: add a4, a5, a3;"
          "m33: add a5, a3, a7;"
          "m34: add a3, a7, s2;"
          "m35: add a7, s2, s3;"
          "m36: add s2, s3, s4;"
          "m37: add s3, s4, s5;"
          "m38: add s4, s5, s6;"
          "m39: add s5, s6, s7;"
          "m40: add s6, s7, s8;"
          "m41: add s7, s8, s9;"
          "m42: add s8, s9, s10;"
          "m43: add s9, s10, s11;"
          "m44: add s10, s11, t3;"
          "m45: add s11, t3, t4;"
          "m46: add t3, t4, t5;"
          "m47: add t4, t5, t6;"
          "m48: add t5, t6, t0;"
          "m49: add t6, t0, t1;"
          "m50: add t0, t1, t2;"
          "m51: add t1, t2, a0;"
          "m52: add t2, a0, a1;"
          "m53: add a0, a1, a2;"
          "m54: add a1, a2, a3;"
          "m55: add a2, a3, a4;"
          "m56: add a3, a4, a5;"
          "m57: add a4, a5, a3;"
          "m58: add a5, a3, a7;"
          "m59: add a3, a7, s2;"
          "m60: add a7, s2, s3;"
          "m61: add s2, s3, s4;"
          "m62: add s3, s4, s5;"
          "m63: add s4, s5, s6;"
          "m64: add s5, s6, s7;"
          "m65: add s6, s7, s8;"
          "m66: add s7, s8, s9;"
          "m67: add s8, s9, s10;"
          "m68: add s9, s10, s11;"
          "m69: add s10, s11, t3;"
          "m70: add s11, t3, t4;"
          "m71: add t3, t4, t5;"
          "m72: add t4, t5, t6;"
          "m73: add t5, t6, t0;"
          "m74: add t6, t0, t1;"
          "m75: add t0, t1, t2;"
          "m76: add t1, t2, a0;"
          "m77: add t2, a0, a1;"
          "m78: add a0, a1, a2;"
          "m79: add a1, a2, a3;"
          "m80: add a2, a3, a4;"
          "m81: add a3, a4, a5;"
          "m82: add a4, a5, a3;"
          "m83: add a5, a3, a7;"
          "m84: add a3, a7, s2;"
          "m85: add a7, s2, s3;"
          "m86: add s2, s3, s4;"
          "m87: add s3, s4, s5;"
          "m88: add s4, s5, s6;"
          "m89: add s5, s6, s7;"
          "m90: add s6, s7, s8;"
          "m91: add s7, s8, s9;"
          "m92: add s8, s9, s10;"
          "m93: add s9, s10, s11;"
          "m94: add s10, s11, t3;"
          "m95: add s11, t3, t4;"
          "m96: add t3, t4, t5;"
          "m97: add t4, t5, t6;"
          "m98: add t5, t6, t0;"
          "m99: add t6, t0, t1;"

                     );
        asm volatile (" ms: addi zero,zero,2");
        return 0;
    }
}
