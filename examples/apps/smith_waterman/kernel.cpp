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
  unsigned char* unpacked_ptr = unpacked;
  for (int i = 0; i < num_packed; i++) {
    int packed_val = packed[i];
    for (int j = 0; j < 16; j++) {
      unsigned char unpacked_val = packed_val >> (30 - 2 * j);
      unpacked_val &= 0x00000003;
      unpacked_ptr[j] = unpacked_val;
    }
    unpacked_ptr += 16;
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

// copy num_words words from DRAM via non-blocking loads
// num_words must be divisible by 16;
template <typename T>
inline void hb_memcpy(const T* src_ptr, const int num_words, T* dst_ptr) {
  const T* src = src_ptr;
  T* dst = dst_ptr;

  for (int i = 0; i < num_words / 16; i++) {
    T tmp00 =  src[0];
    T tmp01 =  src[1];
    T tmp02 =  src[2];
    T tmp03 =  src[3];
    T tmp04 =  src[4];
    T tmp05 =  src[5];
    T tmp06 =  src[6];
    T tmp07 =  src[7];
    T tmp08 =  src[8];
    T tmp09 =  src[9];
    T tmp10 = src[10];
    T tmp11 = src[11];
    T tmp12 = src[12];
    T tmp13 = src[13];
    T tmp14 = src[14];
    T tmp15 = src[15];
    asm volatile("": : :"memory");
    dst[0] = tmp00;
    dst[1] = tmp01;
    dst[2] = tmp02;
    dst[3] = tmp03;
    dst[4] = tmp04;
    dst[5] = tmp05;
    dst[6] = tmp06;
    dst[7] = tmp07;
    dst[8] = tmp08;
    dst[9] = tmp09;
    dst[10] = tmp10;
    dst[11] = tmp11;
    dst[12] = tmp12;
    dst[13] = tmp13;
    dst[14] = tmp14;
    dst[15] = tmp15;
    src += 16;
    dst += 16;
  }
}

inline void load_spm(const unsigned* seqa, const unsigned* seqb,
                     const unsigned* sizea, const unsigned* sizeb,
                     const int num_packed_a, const int num_packed_b,
                     const int N,
                     unsigned* seqa_spm,  unsigned* seqb_spm,
                     unsigned* sizea_spm,  unsigned* sizeb_spm) {
#ifdef MEMCPY_FLAG
  // Transfer sequences
  hb_memcpy(seqa, N * num_packed_a, seqa_spm);
  hb_memcpy(seqb, N * num_packed_b, seqb_spm);

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
  for (int i = 0; i < N * num_packed_a; i++) {
    seqa_spm[i] = seqa[i];
  }
  for (int i = 0; i < N * num_packed_b; i++) {
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

inline void align(const unsigned length, const unsigned width,
                  const unsigned char* seqa_spm_ptr,
                  const unsigned char* seqb_spm_ptr,
                  short* E_spm, short* F_spm, short* H_spm, short* H_prev_spm, int* score) {
  // Hyperparameters (match GPGPU-Sim)
  const int match_score    = 1;
  const int mismatch_score = -3;
  const int gap_open       = 3;
  const int gap_extend     = 1;

  // compute 2D DP matrix
  int score_temp = 0;
  const auto mm = [&](const unsigned char a, const unsigned char b){ return (a==b)?match_score:mismatch_score; };
  for(int i = 0; i < 1; i++) {
    for(int j = 1; j < width; j++) {
      E_spm[j] = max(E_spm[j-1] - gap_extend,
                     H_spm[j-1] - gap_open
                     );

      F_spm[j] = 0;

      H_prev_spm[j] = H_spm[j];
      H_spm[j] = max((short)0, E_spm[j], F_spm[j]);
      if (H_spm[j] > score_temp)
        score_temp = H_spm[j];
    }
  }
  for(int i = 1; i < length; i++) {
    unsigned char seqa_val = seqa_spm_ptr[i];
    for(int j = 1; j < width; j++) {
      E_spm[j] = max(E_spm[j-1] - gap_extend,
                     H_spm[j-1] - gap_open
                     );

      F_spm[j] = max(F_spm[j] - gap_extend,
                       H_spm[j] - gap_open
                      );

      H_prev_spm[j] = H_spm[j];
      H_spm[j] = max((short)0, E_spm[j], F_spm[j],
                    (short)(H_prev_spm[j-1] + mm(seqa_val, seqb_spm_ptr[j])));
      if (H_spm[j] > score_temp)
        score_temp = H_spm[j];
      }
  }
  // DRAM write
  *score = score_temp;
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
  int* score
){
        bsg_nonsynth_saif_start();
  profile_start();
  // determine which alignments the tile does
  int tid = get_tid();
  const int SIZEA_MAX_PACKED = (SIZEA_MAX + 15) / 16;
  const int SIZEB_MAX_PACKED = (SIZEB_MAX + 15) / 16;
  const unsigned* seqa_ptr = seqa + tid * N * SIZEA_MAX_PACKED;
  const unsigned* seqb_ptr = seqb + tid * N * SIZEB_MAX_PACKED;
  const unsigned* sizea_ptr = sizea + tid * N;
  const unsigned* sizeb_ptr = sizeb + tid * N;
  int* score_ptr = score + tid * N;

  // Load data to SPM
  unsigned seqa_packed_spm[N*SIZEA_MAX_PACKED];
  unsigned seqb_packed_spm[N*SIZEB_MAX_PACKED];
  unsigned length[N];
  unsigned width[N];
  load_spm(seqa_ptr, seqb_ptr, sizea_ptr, sizeb_ptr, SIZEA_MAX_PACKED, SIZEB_MAX_PACKED, N,
           seqa_packed_spm, seqb_packed_spm, length, width);

  // Initialize matrices
  short E_spm[SIZEB_MAX];
  short F_spm[SIZEB_MAX];
  short H_spm[SIZEB_MAX];
  short H_prev_spm[SIZEB_MAX];
  for (int i = 0; i < SIZEB_MAX; i++) {
    E_spm[i] = 0;
    F_spm[i] = 0;
    H_spm[i] = 0;
    H_prev_spm[i] = 0;
  }

  // unpack sequences in SPM
  unsigned char seqa_spm[SIZEA_MAX];
  unsigned char seqb_spm[SIZEB_MAX];
  unsigned* seqa_packed_spm_ptr = seqa_packed_spm;
  unsigned* seqb_packed_spm_ptr = seqb_packed_spm;

  // loop through N alignments
  for (int k = 0; k < N; k++) {
    // unpack
    unpack(seqa_packed_spm_ptr, SIZEA_MAX_PACKED, seqa_spm);
    unpack(seqb_packed_spm_ptr, SIZEB_MAX_PACKED, seqb_spm);

    // compute score
    align(length[k], width[k], seqa_spm, seqb_spm,
          E_spm, F_spm, H_spm, H_prev_spm, score_ptr);

    // move to next sequence
    seqa_packed_spm_ptr += SIZEA_MAX_PACKED;
    seqb_packed_spm_ptr += SIZEB_MAX_PACKED;
    score_ptr++;
  }
  profile_end();
  sync();
        bsg_nonsynth_saif_end();
}

