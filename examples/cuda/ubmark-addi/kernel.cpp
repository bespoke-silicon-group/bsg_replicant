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
    int  __attribute__ ((noinline)) kernel_addi(int arg) {
        asm volatile(
					"li t0, 0xb05be091;"
					"li t1, 0x6c21b315;"
					"li t2, 0x18550167;"
					"li a0, 0xb6f99b7b;"
					"li a1, 0x70b43e30;"
					"li a2, 0x3de90d4d;"
					"li a3, 0x03c204b3;"
					"li a4, 0xa8fc90a7;"
					"li a5, 0xc08f7932;"
					"li a3, 0x999c081d;"
					"li a7, 0x164d2bbc;"
					"li s2, 0xc464f381;"
					"li s3, 0x4b5fdefe;"
					"li s4, 0xb9ebe70d;"
					"li s5, 0x106d9dc5;"
					"li s6, 0xea9f8c6a;"
					"li s7, 0xe1e1aac0;"
					"li s8, 0x1e710925;"
					"li s9, 0x7692e692;"
					"li s10, 0xc91326b3;"
					"li s11, 0x2894c3c0;"
					"li t3, 0x7d19c505;"
					"li t4, 0x77b22f16;"
					"li t5, 0x79cacb49;"
					"li t6, 0x56f99d8e;"

                       );
        asm volatile ("addi zero,zero,1;");
        asm volatile (
					"m0: addi t0, t1, 0x1e1;"
					"m1: addi t1, t2, 0x1d3;"
					"m2: addi t2, a0, 0x150;"
					"m3: addi a0, a1, 0x7ba;"
					"m4: addi a1, a2, 0x7d6;"
					"m5: addi a2, a3, 0x552;"
					"m6: addi a3, a4, 0x339;"
					"m7: addi a4, a5, 0x341;"
					"m8: addi a5, a3, 0x65d;"
					"m9: addi a3, a7, 0x7b2;"
					"m10: addi a7, s2, 0x02b;"
					"m11: addi s2, s3, 0x4c4;"
					"m12: addi s3, s4, 0x566;"
					"m13: addi s4, s5, 0x2df;"
					"m14: addi s5, s6, 0x10e;"
					"m15: addi s6, s7, 0x50e;"
					"m16: addi s7, s8, 0x0da;"
					"m17: addi s8, s9, 0x4a5;"
					"m18: addi s9, s10, 0x7a8;"
					"m19: addi s10, s11, 0x1f2;"
					"m20: addi s11, t3, 0x35b;"
					"m21: addi t3, t4, 0x1af;"
					"m22: addi t4, t5, 0x23b;"
					"m23: addi t5, t6, 0x3a9;"
					"m24: addi t6, t0, 0x4ea;"
					"m25: addi t0, t1, 0x5ea;"
					"m26: addi t1, t2, 0x541;"
					"m27: addi t2, a0, 0x304;"
					"m28: addi a0, a1, 0x69e;"
					"m29: addi a1, a2, 0x435;"
					"m30: addi a2, a3, 0x14f;"
					"m31: addi a3, a4, 0x3dd;"
					"m32: addi a4, a5, 0x66e;"
					"m33: addi a5, a3, 0x426;"
					"m34: addi a3, a7, 0x43b;"
					"m35: addi a7, s2, 0x5d8;"
					"m36: addi s2, s3, 0x058;"
					"m37: addi s3, s4, 0x2c8;"
					"m38: addi s4, s5, 0x23b;"
					"m39: addi s5, s6, 0x79d;"
					"m40: addi s6, s7, 0x61c;"
					"m41: addi s7, s8, 0x463;"
					"m42: addi s8, s9, 0x763;"
					"m43: addi s9, s10, 0x2d9;"
					"m44: addi s10, s11, 0x36e;"
					"m45: addi s11, t3, 0x133;"
					"m46: addi t3, t4, 0x264;"
					"m47: addi t4, t5, 0x292;"
					"m48: addi t5, t6, 0x3c7;"
					"m49: addi t6, t0, 0x04c;"
					"m50: addi t0, t1, 0x533;"
					"m51: addi t1, t2, 0x290;"
					"m52: addi t2, a0, 0x5ea;"
					"m53: addi a0, a1, 0x2f6;"
					"m54: addi a1, a2, 0x5a6;"
					"m55: addi a2, a3, 0x7af;"
					"m56: addi a3, a4, 0x0f7;"
					"m57: addi a4, a5, 0x6bf;"
					"m58: addi a5, a3, 0x4df;"
					"m59: addi a3, a7, 0x589;"
					"m60: addi a7, s2, 0x077;"
					"m61: addi s2, s3, 0x49b;"
					"m62: addi s3, s4, 0x255;"
					"m63: addi s4, s5, 0x0a1;"
					"m64: addi s5, s6, 0x6cb;"
					"m65: addi s6, s7, 0x4dd;"
					"m66: addi s7, s8, 0x412;"
					"m67: addi s8, s9, 0x524;"
					"m68: addi s9, s10, 0x353;"
					"m69: addi s10, s11, 0x709;"
					"m70: addi s11, t3, 0x2dc;"
					"m71: addi t3, t4, 0x6a7;"
					"m72: addi t4, t5, 0x5b8;"
					"m73: addi t5, t6, 0x78a;"
					"m74: addi t6, t0, 0x757;"
					"m75: addi t0, t1, 0x09d;"
					"m76: addi t1, t2, 0x4a0;"
					"m77: addi t2, a0, 0x302;"
					"m78: addi a0, a1, 0x24d;"
					"m79: addi a1, a2, 0x468;"
					"m80: addi a2, a3, 0x0d4;"
					"m81: addi a3, a4, 0x6b4;"
					"m82: addi a4, a5, 0x7f0;"
					"m83: addi a5, a3, 0x62f;"
					"m84: addi a3, a7, 0x718;"
					"m85: addi a7, s2, 0x7e4;"
					"m86: addi s2, s3, 0x621;"
					"m87: addi s3, s4, 0x18e;"
					"m88: addi s4, s5, 0x49a;"
					"m89: addi s5, s6, 0x5c1;"
					"m90: addi s6, s7, 0x711;"
					"m91: addi s7, s8, 0x39f;"
					"m92: addi s8, s9, 0x4f3;"
					"m93: addi s9, s10, 0x47e;"
					"m94: addi s10, s11, 0x70e;"
					"m95: addi s11, t3, 0x0db;"
					"m96: addi t3, t4, 0x5e3;"
					"m97: addi t4, t5, 0x3ac;"
					"m98: addi t5, t6, 0x0f3;"
					"m99: addi t6, t0, 0x65a;"

                     );
        asm volatile (" ms: addi zero,zero,2");
        return 0;
    }
}
