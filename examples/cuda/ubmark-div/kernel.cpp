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
    int  __attribute__ ((noinline)) kernel_div(int arg) {
        asm volatile(
          "li t0, 0xca662f21;"
          "li t1, 0x4b5efeea;"
          "li t2, 0xc00557b1;"
          "li a0, 0xfd3493f0;"
          "li a1, 0x2f1b2059;"
          "li a2, 0xa6a87a9b;"
          "li a3, 0xfbeb0e07;"
          "li a4, 0xa6f870a6;"
          "li a5, 0xb6e7a36e;"
          "li a3, 0x1e533d5f;"
          "li a7, 0xb2ae179c;"
          "li s2, 0xfb1c500d;"
          "li s3, 0x4de5c779;"
          "li s4, 0x7124f600;"
          "li s5, 0x32badfc7;"
          "li s6, 0xaf09f743;"
          "li s7, 0x8e440494;"
          "li s8, 0x4352d7c6;"
          "li s9, 0x62394b42;"
          "li s10, 0xc6d5050d;"
          "li s11, 0x43783dc0;"
          "li t3, 0xf9aac162;"
          "li t4, 0xb347ca6a;"
          "li t5, 0x9f5b9ed5;"
          "li t6, 0xfe627ea9;"

                       );
        asm volatile ("addi zero,zero,1;");
        asm volatile (
          "m0: div t0, t1, t2;"
          "m1: div t1, t2, a0;"
          "m2: div t2, a0, a1;"
          "m3: div a0, a1, a2;"
          "m4: div a1, a2, a3;"
          "m5: div a2, a3, a4;"
          "m6: div a3, a4, a5;"
          "m7: div a4, a5, a3;"
          "m8: div a5, a3, a7;"
          "m9: div a3, a7, s2;"
          "m10: div a7, s2, s3;"
          "m11: div s2, s3, s4;"
          "m12: div s3, s4, s5;"
          "m13: div s4, s5, s6;"
          "m14: div s5, s6, s7;"
          "m15: div s6, s7, s8;"
          "m16: div s7, s8, s9;"
          "m17: div s8, s9, s10;"
          "m18: div s9, s10, s11;"
          "m19: div s10, s11, t3;"
          "m20: div s11, t3, t4;"
          "m21: div t3, t4, t5;"
          "m22: div t4, t5, t6;"
          "m23: div t5, t6, t0;"
          "m24: div t6, t0, t1;"
          "m25: div t0, t1, t2;"
          "m26: div t1, t2, a0;"
          "m27: div t2, a0, a1;"
          "m28: div a0, a1, a2;"
          "m29: div a1, a2, a3;"
          "m30: div a2, a3, a4;"
          "m31: div a3, a4, a5;"
          "m32: div a4, a5, a3;"
          "m33: div a5, a3, a7;"
          "m34: div a3, a7, s2;"
          "m35: div a7, s2, s3;"
          "m36: div s2, s3, s4;"
          "m37: div s3, s4, s5;"
          "m38: div s4, s5, s6;"
          "m39: div s5, s6, s7;"
          "m40: div s6, s7, s8;"
          "m41: div s7, s8, s9;"
          "m42: div s8, s9, s10;"
          "m43: div s9, s10, s11;"
          "m44: div s10, s11, t3;"
          "m45: div s11, t3, t4;"
          "m46: div t3, t4, t5;"
          "m47: div t4, t5, t6;"
          "m48: div t5, t6, t0;"
          "m49: div t6, t0, t1;"
          "m50: div t0, t1, t2;"
          "m51: div t1, t2, a0;"
          "m52: div t2, a0, a1;"
          "m53: div a0, a1, a2;"
          "m54: div a1, a2, a3;"
          "m55: div a2, a3, a4;"
          "m56: div a3, a4, a5;"
          "m57: div a4, a5, a3;"
          "m58: div a5, a3, a7;"
          "m59: div a3, a7, s2;"
          "m60: div a7, s2, s3;"
          "m61: div s2, s3, s4;"
          "m62: div s3, s4, s5;"
          "m63: div s4, s5, s6;"
          "m64: div s5, s6, s7;"
          "m65: div s6, s7, s8;"
          "m66: div s7, s8, s9;"
          "m67: div s8, s9, s10;"
          "m68: div s9, s10, s11;"
          "m69: div s10, s11, t3;"
          "m70: div s11, t3, t4;"
          "m71: div t3, t4, t5;"
          "m72: div t4, t5, t6;"
          "m73: div t5, t6, t0;"
          "m74: div t6, t0, t1;"
          "m75: div t0, t1, t2;"
          "m76: div t1, t2, a0;"
          "m77: div t2, a0, a1;"
          "m78: div a0, a1, a2;"
          "m79: div a1, a2, a3;"
          "m80: div a2, a3, a4;"
          "m81: div a3, a4, a5;"
          "m82: div a4, a5, a3;"
          "m83: div a5, a3, a7;"
          "m84: div a3, a7, s2;"
          "m85: div a7, s2, s3;"
          "m86: div s2, s3, s4;"
          "m87: div s3, s4, s5;"
          "m88: div s4, s5, s6;"
          "m89: div s5, s6, s7;"
          "m90: div s6, s7, s8;"
          "m91: div s7, s8, s9;"
          "m92: div s8, s9, s10;"
          "m93: div s9, s10, s11;"
          "m94: div s10, s11, t3;"
          "m95: div s11, t3, t4;"
          "m96: div t3, t4, t5;"
          "m97: div t4, t5, t6;"
          "m98: div t5, t6, t0;"
          "m99: div t6, t0, t1;"

                     );
        asm volatile (" ms: addi zero,zero,2");
        return 0;
    }
}
