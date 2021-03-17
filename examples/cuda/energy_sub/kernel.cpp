//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_energy_sub(int *A, int *B, int *C, int N, int block_size) {
  //------------------------------------
  __asm__ __volatile__ (
                         "addi sp, sp, -48;"
                         "sw   s0, 44(sp);"
                         "sw   s1, 40(sp);"
                         "sw   s2, 36(sp);"
                         "sw   s3, 32(sp);"
                         "sw   s4, 28(sp);"
                         "sw   s5, 24(sp);"
                         "sw   s6, 20(sp);"
                         "sw   s7, 16(sp);"
                         "sw   s8, 12(sp);"
                         "sw   s9, 8(sp);"
                         "sw   s10, 4(sp);"
                         "sw   s11, 0(sp);"
                        );
  __asm__ __volatile__ (
                         "lui t0,0xe39c;"
                         "lui t1,0x95e93;"
                         "lui t2,0x896f8;"
                         "lui s0,0x55f11;"
                         "lui s1,0xe2161;"
                         "lui a0,0x8813b;"
                         "lui a1,0xb5c8b;"
                         "lui a2,0xedcb2;"
                         "lui a3,0x144dd;"
                         "lui a4,0x72316;"
                         "lui a5,0x2b181;"
                         "lui a6,0x5e89a;"
                         "lui a7,0xf0591;"
                         "lui s2,0x687f;"
                         "lui s3,0x976;"
                         "lui s4,0x12cd3;"
                         "lui s5,0x6233c;"
                         "lui s6,0x6ebd9;"
                         "lui s7,0xcf00;"
                         "lui s8,0xf49fb;"
                         "lui s9,0xe10b6;"
                         "lui s10,0x2288f;"
                         "lui s11,0xcfa37;"
                         "lui t3,0xd7341;"
                         "lui t4,0xcca30;"
                         "lui t5,0xfd95d;"
                         "lui t6,0x7c8e3;"
                       );
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  // 100 back to back adds
  __asm__ __volatile__ (
                         // add r22, r25, r26;
                          "sub s6,s9,s10;"
                         // add r23, r27, r28;
                          "sub s7,s11,t3;"
                         // add r24, r29, r30;
                          "sub s8,t4,t5;"
                         // add r25, r31, r32;
                          "sub s9,t6,t0;"
                         // add r26, r1, r2;
                          "sub t3,t0,t1;"
                         // add r29, r7, r8;
                          "sub t4,t2,s0;"
                         // add r30, r9, r10;
                          "sub t5,s1,a0;"
                         // add r31, r11, r12;
                          "sub t6,a1,a2;"
                         // add r5, r23, r24;
                          "sub t0,s7,s8;"
                         // add r6, r25, r26;
                          "sub t1,s9,s10;"
                         // add r7, r27, r28;
                          "sub t2,s11,t3;"
                         // add r8, r29, r30;
                          "sub s0,t4,t5;"
                         // add r9, r31, r32;
                          "sub s1,t6,t0;"
                         // add r10, r1, r2;
                          "sub a2,t0,t1;"
                         // add r13, r7, r8;
                          "sub a3,t2,s0;"
                         // add r14, r9, r10;
                          "sub a4,s1,a0;"
                         // add r15, r11, r12;
                          "sub a5,a1,a2;"
                         // add r16, r13, r14;
                          "sub a6,a3,a4;"
                         // add r17, r15, r16;
                          "sub a7,a5,a6;"
                         // add r18, r17, r18;
                          "sub s2,a7,s2;"
                         // add r19, r19, r20;
                          "sub s3,s3,s4;"
                         // add r20, r21, r22;
                          "sub s4,s5,s6;"
                         // add r21, r23, r24;
                          "sub s5,s7,s8;"
                         // add r22, r25, r26;
                          "sub s6,s9,s10;"
                         // add r23, r27, r28;
                          "sub s7,s11,t3;"
                         // add r24, r29, r30;
                          "sub s8,t4,t5;"
                         // add r25, r31, r32;
                          "sub s9,t6,t0;"
                         // add r26, r1, r2;
                          "sub t3,t0,t1;"
                         // add r29, r7, r8;
                          "sub t4,t2,s0;"
                         // add r30, r9, r10;
                          "sub t5,s1,a0;"
                         // add r31, r11, r12;
                          "sub t6,a1,a2;"
                         // add r5, r23, r24;
                          "sub t0,s7,s8;"
                         // add r6, r25, r26;
                          "sub t1,s9,s10;"
                         // add r7, r27, r28;
                          "sub t2,s11,t3;"
                         // add r8, r29, r30;
                          "sub s0,t4,t5;"
                         // add r9, r31, r32;
                          "sub s1,t6,t0;"
                         // add r10, r1, r2;
                          "sub a2,t0,t1;"
                         // add r13, r7, r8;
                          "sub a3,t2,s0;"
                         // add r14, r9, r10;
                          "sub a4,s1,a0;"
                         // add r15, r11, r12;
                          "sub a5,a1,a2;"
                         // add r16, r13, r14;
                          "sub a6,a3,a4;"
                         // add r17, r15, r16;
                          "sub a7,a5,a6;"
                         // add r18, r17, r18;
                          "sub s2,a7,s2;"
                         // add r19, r19, r20;
                          "sub s3,s3,s4;"
                         // add r20, r21, r22;
                          "sub s4,s5,s6;"
                         // add r21, r23, r24;
                          "sub s5,s7,s8;"
                         // add r22, r25, r26;
                          "sub s6,s9,s10;"
                         // add r23, r27, r28;
                          "sub s7,s11,t3;"
                         // add r24, r29, r30;
                          "sub s8,t4,t5;"
                         // add r25, r31, r32;
                          "sub s9,t6,t0;"
                         // add r26, r1, r2;
                          "sub t3,t0,t1;"
                         // add r29, r7, r8;
                          "sub t4,t2,s0;"
                         // add r30, r9, r10;
                          "sub t5,s1,a0;"
                         // add r31, r11, r12;
                          "sub t6,a1,a2;"
                         // add r5, r23, r24;
                          "sub t0,s7,s8;"
                         // add r6, r25, r26;
                          "sub t1,s9,s10;"
                         // add r7, r27, r28;
                          "sub t2,s11,t3;"
                         // add r8, r29, r30;
                          "sub s0,t4,t5;"
                         // add r9, r31, r32;
                          "sub s1,t6,t0;"
                         // add r10, r1, r2;
                          "sub a2,t0,t1;"
                         // add r13, r7, r8;
                          "sub a3,t2,s0;"
                         // add r14, r9, r10;
                          "sub a4,s1,a0;"
                         // add r15, r11, r12;
                          "sub a5,a1,a2;"
                         // add r16, r13, r14;
                          "sub a6,a3,a4;"
                         // add r17, r15, r16;
                          "sub a7,a5,a6;"
                         // add r18, r17, r18;
                          "sub s2,a7,s2;"
                         // add r19, r19, r20;
                          "sub s3,s3,s4;"
                         // add r20, r21, r22;
                          "sub s4,s5,s6;"
                         // add r21, r23, r24;
                          "sub s5,s7,s8;"
                         // add r22, r25, r26;
                          "sub s6,s9,s10;"
                         // add r23, r27, r28;
                          "sub s7,s11,t3;"
                         // add r24, r29, r30;
                          "sub s8,t4,t5;"
                         // add r25, r31, r32;
                          "sub s9,t6,t0;"
                         // add r26, r1, r2;
                          "sub t3,t0,t1;"
                         // add r29, r7, r8;
                          "sub t4,t2,s0;"
                         // add r30, r9, r10;
                          "sub t5,s1,a0;"
                         // add r31, r11, r12;
                          "sub t6,a1,a2;"
                         // add r5, r23, r24;
                          "sub t0,s7,s8;"
                         // add r6, r25, r26;
                          "sub t1,s9,s10;"
                         // add r7, r27, r28;
                          "sub t2,s11,t3;"
                         // add r8, r29, r30;
                          "sub s0,t4,t5;"
                         // add r9, r31, r32;
                          "sub s1,t6,t0;"
                         // add r10, r1, r2;
                          "sub a2,t0,t1;"
                         // add r13, r7, r8;
                          "sub a3,t2,s0;"
                         // add r14, r9, r10;
                          "sub a4,s1,a0;"
                         // add r15, r11, r12;
                          "sub a5,a1,a2;"
                         // add r16, r13, r14;
                          "sub a6,a3,a4;"
                         // add r17, r15, r16;
                          "sub a7,a5,a6;"
                         // add r18, r17, r18;
                          "sub s2,a7,s2;"
                         // add r19, r19, r20;
                          "sub s3,s3,s4;"
                         // add r20, r21, r22;
                          "sub s4,s5,s6;"
                         // add r21, r23, r24;
                          "sub s5,s7,s8;"
                         // add r22, r25, r26;
                          "sub s6,s9,s10;"
                         // add r23, r27, r28;
                          "sub s7,s11,t3;"
                         // add r24, r29, r30;
                          "sub s8,t4,t5;"
                         // add r25, r31, r32;
                          "sub s9,t6,t0;"
                         // add r26, r1, r2;
                          "sub t3,t0,t1;"
                         // add r29, r7, r8;
                          "sub t4,t2,s0;"
                         // add r30, r9, r10;
                          "sub t5,s1,a0;"
                         // add r31, r11, r12;
                          "sub t6,a1,a2;"
                       );
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  __asm__ __volatile__ (
                         "lw   s0, 44(sp);"
                         "lw   s1, 40(sp);"
                         "lw   s2, 36(sp);"
                         "lw   s3, 32(sp);"
                         "lw   s4, 28(sp);"
                         "lw   s5, 24(sp);"
                         "lw   s6, 20(sp);"
                         "lw   s7, 16(sp);"
                         "lw   s8, 12(sp);"
                         "lw   s9, 8(sp);"
                         "lw   s10, 4(sp);"
                         "lw   s11, 0(sp);"
                         "addi sp, sp, 48;"
                        );
	barrier.sync();

	return 0;
}
