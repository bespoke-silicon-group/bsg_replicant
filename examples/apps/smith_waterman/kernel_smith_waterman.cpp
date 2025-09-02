#define MEMCPY_FLAG
#define HB
//#define DEBUG

#ifdef HB
#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_tile_group_barrier.hpp"

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
#else
#include "kernel_smith_waterman.hpp"
#endif

template <typename T>
inline T max(T a, T b)
{
  if (a > b)
    return a;
  else
    return b;
}

template <typename T>
inline T max(T a, T b, T c, T d)
{
  return max(max(a, b), max(c, d));
}

template <typename T>
inline T max(T a, T b, T c)
{
  return max(max(a, b), c);
}

inline void unpack(const unsigned* packed, const int num_packed, unsigned char* unpacked) {
  for (int i = 0; i < num_packed; i++) {
    int packed_val = packed[i];
    for (int j = 0; j < 16; j++) {
      unsigned char unpacked_val = packed_val >> (30 - 2 * j);
      unpacked_val &= 0x00000003;
      unpacked[j] = unpacked_val;
    }
    unpacked += 16;
  }
}

inline void profile_start(){
#ifdef HB
  bsg_cuda_print_stat_kernel_start();
#endif
}

inline void profile_end(){
#ifdef HB
  bsg_cuda_print_stat_kernel_end();
#endif
}

inline void sync(){
#ifdef HB
  barrier.sync();
#endif
}

inline void debug_printf(int tid, int k, int N, int i, int length){
#ifdef HB
#ifdef DEBUG
  bsg_printf("[Tile %d] Alignment %d/%d, Row %d/%d\n", tid, k, N, i, length);
#endif
#endif
}

inline void load_spm(const unsigned* seqa, const unsigned* seqb,
                     const unsigned* sizea, const unsigned* sizeb,
                     const int num_packed_a, const int num_packed_b,
                     const int N,
                     unsigned* seqa_spm,  unsigned* seqb_spm,
                     unsigned* sizea_spm,  unsigned* sizeb_spm) {
#ifdef MEMCPY_FLAG
  unsigned seqa_temp0 = seqa[0];
  unsigned seqa_temp1 = seqa[1];
  unsigned seqa_temp2 = seqa[2];
  unsigned seqa_temp3 = seqa[3];
  unsigned seqa_temp4 = seqa[4];
  unsigned seqa_temp5 = seqa[5];
  unsigned seqa_temp6 = seqa[6];
  unsigned seqa_temp7 = seqa[7];
  unsigned seqb_temp0 = seqb[0];
  unsigned seqb_temp1 = seqb[1];
  unsigned seqb_temp2 = seqb[2];
  unsigned seqb_temp3 = seqb[3];
  unsigned seqb_temp4 = seqb[4];
  unsigned seqb_temp5 = seqb[5];
  unsigned seqb_temp6 = seqb[6];
  unsigned seqb_temp7 = seqb[7];
  asm volatile("": : :"memory");
  seqa_spm[0] = seqa_temp0;
  seqa_spm[1] = seqa_temp1;
  seqa_spm[2] = seqa_temp2;
  seqa_spm[3] = seqa_temp3;
  seqa_spm[4] = seqa_temp4;
  seqa_spm[5] = seqa_temp5;
  seqa_spm[6] = seqa_temp6;
  seqa_spm[7] = seqa_temp7;
  seqb_spm[0] = seqb_temp0;
  seqb_spm[1] = seqb_temp1;
  seqb_spm[2] = seqb_temp2;
  seqb_spm[3] = seqb_temp3;
  seqb_spm[4] = seqb_temp4;
  seqb_spm[5] = seqb_temp5;
  seqb_spm[6] = seqb_temp6;
  seqb_spm[7] = seqb_temp7;

  // Transfer sequence lengths
  unsigned sizea_temp0 = sizea[0];
  unsigned sizea_temp1 = sizea[1];
  unsigned sizea_temp2 = sizea[2];
  unsigned sizea_temp3 = sizea[3];
  unsigned sizeb_temp0 = sizeb[0];
  unsigned sizeb_temp1 = sizeb[1];
  unsigned sizeb_temp2 = sizeb[2];
  unsigned sizeb_temp3 = sizeb[3];
  asm volatile("": : :"memory");
  sizea_spm[0] = sizea_temp0;
  sizea_spm[1] = sizea_temp1;
  sizea_spm[2] = sizea_temp2;
  sizea_spm[3] = sizea_temp3;
  sizeb_spm[0] = sizeb_temp0;
  sizeb_spm[1] = sizeb_temp1;
  sizeb_spm[2] = sizeb_temp2;
  sizeb_spm[3] = sizeb_temp3;
#else
  for (int i = 0; i < num_packed_a; i++) {
    seqa_spm[i] = seqa[i];
  }
  for (int i = 0; i < num_packed_b; i++) {
    seqb_spm[i] = seqb[i];
  }

  // load sizes of sequences to SPM
  for (int k = 0; k < N; k++) {
    sizea_spm[k] = sizea[k];
    sizeb_spm[k] = sizeb[k];
  }
#endif
}

inline int get_tid(){
#ifdef HB
  return bsg_x * bsg_tiles_Y + bsg_y;
#else
  return 0;
#endif
}

#ifdef HB
extern "C" __attribute__ ((noinline))
#endif
void kernel_smith_waterman(
  const int N,
  const int SIZEA_MAX,
  const int SIZEB_MAX,
  const unsigned* seqa,
  const unsigned* seqb,
  const unsigned* sizea,
  const unsigned* sizeb,
  unsigned* score
){
  profile_start();
  // Hyperparameters (match GPGPU-Sim)
  const int match_score    = 1;
  const int mismatch_score = -3;
  const int gap_open       = 3;
  const int gap_extend     = 1;

  // determine which alignments the tile does
  int tid = get_tid();
  const int SIZEA_MAX_PACKED = (SIZEA_MAX + 15) / 16;
  const int SIZEB_MAX_PACKED = (SIZEB_MAX + 15) / 16;
  const unsigned* seqa_ptr = seqa + tid * N * SIZEA_MAX_PACKED;
  const unsigned* seqb_ptr = seqb + tid * N * SIZEB_MAX_PACKED;
  const unsigned* sizea_ptr = sizea + tid * N;
  const unsigned* sizeb_ptr = sizeb + tid * N;
  unsigned* score_ptr = score + tid * N;

  // Load data to SPM
  unsigned seqa_packed_spm[N*SIZEA_MAX_PACKED];
  unsigned seqb_packed_spm[N*SIZEB_MAX_PACKED];
  unsigned length[N];
  unsigned width[N];
  load_spm(seqa_ptr, seqb_ptr, sizea_ptr, sizeb_ptr, SIZEA_MAX_PACKED, SIZEB_MAX_PACKED, N,
           seqa_packed_spm, seqb_packed_spm, length, width);

  // Unpack sequences in SPM
  unsigned char seqa_spm[N*SIZEA_MAX];
  unsigned char seqb_spm[N*SIZEB_MAX];
  unpack(seqa_packed_spm, N * SIZEA_MAX_PACKED, seqa_spm);
  unpack(seqb_packed_spm, N * SIZEB_MAX_PACKED, seqb_spm);

  unsigned char* seqa_spm_ptr = seqa_spm;
  unsigned char* seqb_spm_ptr = seqb_spm;

  int E_spm[SIZEB_MAX];
  int F_spm[SIZEB_MAX];
  int H_spm[SIZEB_MAX];
  int H_prev_spm[SIZEB_MAX];
  unsigned score_temp;
  for (int i = 0; i < SIZEB_MAX; i++) {
    E_spm[i] = 0;
    F_spm[i] = 0;
    H_spm[i] = 0;
    H_prev_spm[i] = 0;
  }

  // loop through N alignments
  for (int k = 0; k < N; k++) {
    const auto mm = [&](const unsigned char a, const unsigned char b){ return (a==b)?match_score:mismatch_score; };

    // compute 2D DP matrix
    score_temp = 0;
    for(int i = 0; i < 1; i++) {
      debug_printf(tid, k, N, i, length[k]);
      for(int j = 1; j < width[k]; j++) {
        E_spm[j] = max(E_spm[j-1] - gap_extend,
                       H_spm[j-1] - gap_open
                       );

        F_spm[j] = 0;

        H_prev_spm[j] = H_spm[j];
        H_spm[j] = max(0, E_spm[j], F_spm[j]);
        if (H_spm[j] > score_temp)
          score_temp = H_spm[j];
      }
    }
    for(int i = 1; i < length[k]; i++) {
      debug_printf(tid, k, N, i, length[k]);
      unsigned char seqa_val = seqa_spm_ptr[i];
      for(int j = 1; j < width[k]; j++) {
        E_spm[j] = max(E_spm[j-1] - gap_extend,
                       H_spm[j-1] - gap_open
                       );

        F_spm[j] = max(F_spm[j] - gap_extend,
                       H_spm[j] - gap_open
                      );

        H_prev_spm[j] = H_spm[j];
        H_spm[j] = max(0, E_spm[j], F_spm[j],
                      H_prev_spm[j-1] + mm(seqa_val, seqb_spm_ptr[j]));
        if (H_spm[j] > score_temp)
          score_temp = H_spm[j];
      }
    }
    score_ptr[k] = score_temp;
    seqa_spm_ptr += SIZEA_MAX;
    seqb_spm_ptr += SIZEB_MAX;
  }
  profile_end();
  sync();
}

