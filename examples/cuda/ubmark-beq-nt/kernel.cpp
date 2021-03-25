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
    int  __attribute__ ((noinline)) kernel_beq_nt(int arg) {
        asm volatile(
          "li t0, 0xcc62104e;"
          "li t1, 0x2e5815e6;"
          "li t2, 0xbc6e58d0;"
          "li a0, 0xf9918a03;"
          "li a1, 0x338f9035;"
          "li a2, 0x8f19152d;"
          "li a3, 0xd50a47ab;"
          "li a4, 0xc4b8a403;"
          "li a5, 0x53d5b032;"
          "li a3, 0x5d7002fd;"
          "li a7, 0x553a4c8f;"
          "li s2, 0xdd7a849e;"
          "li s3, 0x97a375ef;"
          "li s4, 0x0dd7df92;"
          "li s5, 0x91caab39;"
          "li s6, 0x70aa1471;"
          "li s7, 0x8ad072c6;"
          "li s8, 0x4b1cae33;"
          "li s9, 0x602c2210;"
          "li s10, 0x0ed2f0b0;"
          "li s11, 0x2cfb0372;"
          "li t3, 0x30c5cbc9;"
          "li t4, 0x8f515121;"
          "li t5, 0x7481e89a;"
          "li t6, 0x5b311564;"

                       );
        asm volatile ("addi zero,zero,1;");
        asm volatile (
          "m0: beq t0, t1, ms;"
          "m1: beq t1, t2, ms;"
          "m2: beq t2, a0, ms;"
          "m3: beq a0, a1, ms;"
          "m4: beq a1, a2, ms;"
          "m5: beq a2, a3, ms;"
          "m6: beq a3, a4, ms;"
          "m7: beq a4, a5, ms;"
          "m8: beq a5, a3, ms;"
          "m9: beq a3, a7, ms;"
          "m10: beq a7, s2, ms;"
          "m11: beq s2, s3, ms;"
          "m12: beq s3, s4, ms;"
          "m13: beq s4, s5, ms;"
          "m14: beq s5, s6, ms;"
          "m15: beq s6, s7, ms;"
          "m16: beq s7, s8, ms;"
          "m17: beq s8, s9, ms;"
          "m18: beq s9, s10, ms;"
          "m19: beq s10, s11, ms;"
          "m20: beq s11, t3, ms;"
          "m21: beq t3, t4, ms;"
          "m22: beq t4, t5, ms;"
          "m23: beq t5, t6, ms;"
          "m24: beq t6, t0, ms;"
          "m25: beq t0, t1, ms;"
          "m26: beq t1, t2, ms;"
          "m27: beq t2, a0, ms;"
          "m28: beq a0, a1, ms;"
          "m29: beq a1, a2, ms;"
          "m30: beq a2, a3, ms;"
          "m31: beq a3, a4, ms;"
          "m32: beq a4, a5, ms;"
          "m33: beq a5, a3, ms;"
          "m34: beq a3, a7, ms;"
          "m35: beq a7, s2, ms;"
          "m36: beq s2, s3, ms;"
          "m37: beq s3, s4, ms;"
          "m38: beq s4, s5, ms;"
          "m39: beq s5, s6, ms;"
          "m40: beq s6, s7, ms;"
          "m41: beq s7, s8, ms;"
          "m42: beq s8, s9, ms;"
          "m43: beq s9, s10, ms;"
          "m44: beq s10, s11, ms;"
          "m45: beq s11, t3, ms;"
          "m46: beq t3, t4, ms;"
          "m47: beq t4, t5, ms;"
          "m48: beq t5, t6, ms;"
          "m49: beq t6, t0, ms;"
          "m50: beq t0, t1, ms;"
          "m51: beq t1, t2, ms;"
          "m52: beq t2, a0, ms;"
          "m53: beq a0, a1, ms;"
          "m54: beq a1, a2, ms;"
          "m55: beq a2, a3, ms;"
          "m56: beq a3, a4, ms;"
          "m57: beq a4, a5, ms;"
          "m58: beq a5, a3, ms;"
          "m59: beq a3, a7, ms;"
          "m60: beq a7, s2, ms;"
          "m61: beq s2, s3, ms;"
          "m62: beq s3, s4, ms;"
          "m63: beq s4, s5, ms;"
          "m64: beq s5, s6, ms;"
          "m65: beq s6, s7, ms;"
          "m66: beq s7, s8, ms;"
          "m67: beq s8, s9, ms;"
          "m68: beq s9, s10, ms;"
          "m69: beq s10, s11, ms;"
          "m70: beq s11, t3, ms;"
          "m71: beq t3, t4, ms;"
          "m72: beq t4, t5, ms;"
          "m73: beq t5, t6, ms;"
          "m74: beq t6, t0, ms;"
          "m75: beq t0, t1, ms;"
          "m76: beq t1, t2, ms;"
          "m77: beq t2, a0, ms;"
          "m78: beq a0, a1, ms;"
          "m79: beq a1, a2, ms;"
          "m80: beq a2, a3, ms;"
          "m81: beq a3, a4, ms;"
          "m82: beq a4, a5, ms;"
          "m83: beq a5, a3, ms;"
          "m84: beq a3, a7, ms;"
          "m85: beq a7, s2, ms;"
          "m86: beq s2, s3, ms;"
          "m87: beq s3, s4, ms;"
          "m88: beq s4, s5, ms;"
          "m89: beq s5, s6, ms;"
          "m90: beq s6, s7, ms;"
          "m91: beq s7, s8, ms;"
          "m92: beq s8, s9, ms;"
          "m93: beq s9, s10, ms;"
          "m94: beq s10, s11, ms;"
          "m95: beq s11, t3, ms;"
          "m96: beq t3, t4, ms;"
          "m97: beq t4, t5, ms;"
          "m98: beq t5, t6, ms;"
          "m99: beq t6, t0, ms;"

                     );
        asm volatile (" ms: addi zero,zero,2");
        return 0;
    }
}
