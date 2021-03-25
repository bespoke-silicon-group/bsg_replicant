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
    int  __attribute__ ((noinline)) kernel_mul(int arg) {
        asm volatile(
          "li t0, 0x8db9d241;"
          "li t1, 0x1aceee5d;"
          "li t2, 0x56d3eb23;"
          "li a0, 0xe687b361;"
          "li a1, 0x86a2241d;"
          "li a2, 0x4320b074;"
          "li a3, 0x1ca4dac1;"
          "li a4, 0x126ecbb8;"
          "li a5, 0xb78f0738;"
          "li a3, 0x332247bd;"
          "li a7, 0x658ea050;"
          "li s2, 0x048e73d0;"
          "li s3, 0x432e9f2c;"
          "li s4, 0xc4a22358;"
          "li s5, 0x698ac53e;"
          "li s6, 0x49de6992;"
          "li s7, 0x4d86c245;"
          "li s8, 0x2566835d;"
          "li s9, 0x1872c7d9;"
          "li s10, 0x5c9bf4a2;"
          "li s11, 0x4316caf4;"
          "li t3, 0x67d34425;"
          "li t4, 0x7037b020;"
          "li t5, 0xa07ea4a5;"
          "li t6, 0xd905db9e;"

                       );
        asm volatile ("addi zero,zero,1;");
        asm volatile (
          "m0: mul t0, t1, t2;"
          "m1: mul t1, t2, a0;"
          "m2: mul t2, a0, a1;"
          "m3: mul a0, a1, a2;"
          "m4: mul a1, a2, a3;"
          "m5: mul a2, a3, a4;"
          "m6: mul a3, a4, a5;"
          "m7: mul a4, a5, a3;"
          "m8: mul a5, a3, a7;"
          "m9: mul a3, a7, s2;"
          "m10: mul a7, s2, s3;"
          "m11: mul s2, s3, s4;"
          "m12: mul s3, s4, s5;"
          "m13: mul s4, s5, s6;"
          "m14: mul s5, s6, s7;"
          "m15: mul s6, s7, s8;"
          "m16: mul s7, s8, s9;"
          "m17: mul s8, s9, s10;"
          "m18: mul s9, s10, s11;"
          "m19: mul s10, s11, t3;"
          "m20: mul s11, t3, t4;"
          "m21: mul t3, t4, t5;"
          "m22: mul t4, t5, t6;"
          "m23: mul t5, t6, t0;"
          "m24: mul t6, t0, t1;"
          "m25: mul t0, t1, t2;"
          "m26: mul t1, t2, a0;"
          "m27: mul t2, a0, a1;"
          "m28: mul a0, a1, a2;"
          "m29: mul a1, a2, a3;"
          "m30: mul a2, a3, a4;"
          "m31: mul a3, a4, a5;"
          "m32: mul a4, a5, a3;"
          "m33: mul a5, a3, a7;"
          "m34: mul a3, a7, s2;"
          "m35: mul a7, s2, s3;"
          "m36: mul s2, s3, s4;"
          "m37: mul s3, s4, s5;"
          "m38: mul s4, s5, s6;"
          "m39: mul s5, s6, s7;"
          "m40: mul s6, s7, s8;"
          "m41: mul s7, s8, s9;"
          "m42: mul s8, s9, s10;"
          "m43: mul s9, s10, s11;"
          "m44: mul s10, s11, t3;"
          "m45: mul s11, t3, t4;"
          "m46: mul t3, t4, t5;"
          "m47: mul t4, t5, t6;"
          "m48: mul t5, t6, t0;"
          "m49: mul t6, t0, t1;"
          "m50: mul t0, t1, t2;"
          "m51: mul t1, t2, a0;"
          "m52: mul t2, a0, a1;"
          "m53: mul a0, a1, a2;"
          "m54: mul a1, a2, a3;"
          "m55: mul a2, a3, a4;"
          "m56: mul a3, a4, a5;"
          "m57: mul a4, a5, a3;"
          "m58: mul a5, a3, a7;"
          "m59: mul a3, a7, s2;"
          "m60: mul a7, s2, s3;"
          "m61: mul s2, s3, s4;"
          "m62: mul s3, s4, s5;"
          "m63: mul s4, s5, s6;"
          "m64: mul s5, s6, s7;"
          "m65: mul s6, s7, s8;"
          "m66: mul s7, s8, s9;"
          "m67: mul s8, s9, s10;"
          "m68: mul s9, s10, s11;"
          "m69: mul s10, s11, t3;"
          "m70: mul s11, t3, t4;"
          "m71: mul t3, t4, t5;"
          "m72: mul t4, t5, t6;"
          "m73: mul t5, t6, t0;"
          "m74: mul t6, t0, t1;"
          "m75: mul t0, t1, t2;"
          "m76: mul t1, t2, a0;"
          "m77: mul t2, a0, a1;"
          "m78: mul a0, a1, a2;"
          "m79: mul a1, a2, a3;"
          "m80: mul a2, a3, a4;"
          "m81: mul a3, a4, a5;"
          "m82: mul a4, a5, a3;"
          "m83: mul a5, a3, a7;"
          "m84: mul a3, a7, s2;"
          "m85: mul a7, s2, s3;"
          "m86: mul s2, s3, s4;"
          "m87: mul s3, s4, s5;"
          "m88: mul s4, s5, s6;"
          "m89: mul s5, s6, s7;"
          "m90: mul s6, s7, s8;"
          "m91: mul s7, s8, s9;"
          "m92: mul s8, s9, s10;"
          "m93: mul s9, s10, s11;"
          "m94: mul s10, s11, t3;"
          "m95: mul s11, t3, t4;"
          "m96: mul t3, t4, t5;"
          "m97: mul t4, t5, t6;"
          "m98: mul t5, t6, t0;"
          "m99: mul t6, t0, t1;"

                     );
        asm volatile (" ms: addi zero,zero,2");
        return 0;
    }
}
