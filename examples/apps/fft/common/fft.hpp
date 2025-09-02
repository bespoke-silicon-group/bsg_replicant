// common_fft.hpp
// FFT utilities and an optimized 256-point radix-2 DIT implementation

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"

#include <complex>

// This library has sinf/cosf functions specialized for the following
// number of points
#define MAX_NUM_POINTS      256
#define MAX_LOG2_NUM_POINTS 8
#define UNROLL 4

#ifdef FFT128
#define NUM_POINTS      128
#define LOG2_NUM_POINTS 7
#else
// By default we provide utilities for 256-point FFT
#define NUM_POINTS      256
#define LOG2_NUM_POINTS 8
#endif

typedef std::complex<float> FP32Complex;

float minus_2pi = -6.283185307f;
float zerof     = 0.0f;

/*******************************************************************************
 * Efficient sinf and cosf implementation
*******************************************************************************/
// NOTE: both sinf and cosf here are specialized for a specific input size
// Return sin(-2*pi*x/NUM_POINTS) and cos(-2*pi*x/NUM_POINTS)

// 65 elements = 260B
float sinf_pi_over_2[(MAX_NUM_POINTS>>2)+1] = {
    0.0000000000000000f,
    0.0245412285229123f,
    0.0490676743274180f,
    0.0735645635996674f,
    0.0980171403295606f,
    0.1224106751992162f,
    0.1467304744553617f,
    0.1709618887603012f,
    0.1950903220161282f,
    0.2191012401568698f,
    0.2429801799032639f,
    0.2667127574748984f,
    0.2902846772544623f,
    0.3136817403988915f,
    0.3368898533922201f,
    0.3598950365349881f,
    0.3826834323650898f,
    0.4052413140049899f,
    0.4275550934302821f,
    0.4496113296546065f,
    0.4713967368259976f,
    0.4928981922297840f,
    0.5141027441932217f,
    0.5349976198870972f,
    0.5555702330196022f,
    0.5758081914178453f,
    0.5956993044924334f,
    0.6152315905806268f,
    0.6343932841636455f,
    0.6531728429537768f,
    0.6715589548470183f,
    0.6895405447370668f,
    0.7071067811865475f,
    0.7242470829514669f,
    0.7409511253549591f,
    0.7572088465064845f,
    0.7730104533627370f,
    0.7883464276266062f,
    0.8032075314806448f,
    0.8175848131515837f,
    0.8314696123025452f,
    0.8448535652497070f,
    0.8577286100002721f,
    0.8700869911087113f,
    0.8819212643483549f,
    0.8932243011955153f,
    0.9039892931234433f,
    0.9142097557035307f,
    0.9238795325112867f,
    0.9329927988347388f,
    0.9415440651830208f,
    0.9495281805930367f,
    0.9569403357322089f,
    0.9637760657954398f,
    0.9700312531945440f,
    0.9757021300385286f,
    0.9807852804032304f,
    0.9852776423889412f,
    0.9891765099647810f,
    0.9924795345987100f,
    0.9951847266721968f,
    0.9972904566786902f,
    0.9987954562051724f,
    0.9996988186962042f,
    1.0000000000000000f
};

inline float
opt_fft_sinf(int x) {
    const int No2 = MAX_NUM_POINTS >> 1;
    const int No4 = MAX_NUM_POINTS >> 2;
    if ((x >= No4) && (x < No2)) {
        x = No2 - x;
    }
    else if (x >= No2) {
        bsg_fail();
    }
    // consult lookup table for sinf(x)
    return -sinf_pi_over_2[x];
}

inline float
opt_fft_cosf(int x) {
    const int No2 = MAX_NUM_POINTS >> 1;
    const int No4 = MAX_NUM_POINTS >> 2;
    if ((x >= No4) && (x < No2)) {
        return -sinf_pi_over_2[x-No4];
    }
    else if (x >= No2) {
        bsg_fail();
    } else {
        return sinf_pi_over_2[No4-x];
    }
}

/* End of sinf and cosf implementation */

inline void
debug_print_complex(const FP32Complex *list, const int N, const char *msg) {
    if (__bsg_id == 0) {
        bsg_printf("%s\n", msg, list);
        for(int i = 0; i < N; i++) {
            float rr = list[i].real();
            float ii = list[i].imag();
            bsg_printf("(0x%08X)+(0x%08X)i ", *(int*)&rr, *(int*)&ii);
        }
        bsg_printf("\n");
    }
}

inline void
debug_print_complex_per_tile(const FP32Complex *list, const int N, const char *msg) {
    bsg_printf("[ID:%d] %s\n", __bsg_id, msg);
    for(int i = 0; i < N; i++) {
        float rr = list[i].real();
        float ii = list[i].imag();
        bsg_printf("[ID:%d] (0x%08X)+(0x%08X)i ", __bsg_id, *(int*)&rr, *(int*)&ii);
    }
    bsg_printf("\n");
}

inline void
opt_data_transfer(FP32Complex *dst, const FP32Complex *src, const int N) {
    int i = 0;
    for (; i < N-3; i += 4) {
        register FP32Complex tmp0 = src[i    ];
        register FP32Complex tmp1 = src[i + 1];
        register FP32Complex tmp2 = src[i + 2];
        register FP32Complex tmp3 = src[i + 3];
        asm volatile("": : :"memory");
        dst[i    ] = tmp0;
        dst[i + 1] = tmp1;
        dst[i + 2] = tmp2;
        dst[i + 3] = tmp3;
    }
    // fixup
    for (; i < N; i++)
        dst[i] = src[i];
}

inline void
opt_data_transfer_src_strided(FP32Complex *dst, const FP32Complex *src, const int stride, const int N) {
    int i = 0, strided_i = 0;
    for (; i < N-3; i += 4, strided_i += 4*stride) {
        register FP32Complex tmp0 = src[strided_i           ];
        register FP32Complex tmp1 = src[strided_i + stride  ];
        register FP32Complex tmp2 = src[strided_i + stride*2];
        register FP32Complex tmp3 = src[strided_i + stride*3];
        asm volatile("": : :"memory");
        dst[i    ] = tmp0;
        dst[i + 1] = tmp1;
        dst[i + 2] = tmp2;
        dst[i + 3] = tmp3;
    }
    // fixup
    for (; i < N; i++, strided_i += stride) {
        dst[i] = src[strided_i];
    }
}

inline void
opt_data_transfer_dst_strided(FP32Complex *dst, const FP32Complex *src, const int stride, const int N) {
    int i = 0, strided_i = 0;
    for (; i < N-3; i += 4, strided_i += 4*stride) {
        register FP32Complex tmp0 = src[i    ];
        register FP32Complex tmp1 = src[i + 1];
        register FP32Complex tmp2 = src[i + 2];
        register FP32Complex tmp3 = src[i + 3];
        asm volatile("": : :"memory");
        dst[strided_i              ] = tmp0;
        dst[strided_i + stride  ] = tmp1;
        dst[strided_i + stride*2] = tmp2;
        dst[strided_i + stride*3] = tmp3;
    }
    // fixup
    for (; i < N; i++, strided_i += stride) {
        dst[strided_i] = src[i];
    }
}

inline void
opt_bit_reverse(FP32Complex *list, const int N) {
    // Efficient bit reverse
    // http://wwwa.pikara.ne.jp/okojisan/otfft-en/cooley-tukey.html
    // The idea is to perform a reversed binary +1. The inner for loop
    // executes M times, where M is the number of carries in the
    // reversed binary +1 operation.
    for (int i = 0, j = 1; j < N-1; j++) {
        for (int k = N >> 1; k > (i ^= k); k >>= 1);
        // after the for loop above i is bit-reversed j
        if (i < j) {
            register FP32Complex tmp = list[i];
            list[i] = list[j];
            list[j] = tmp;
        }
    }
}

// In-place implementation
inline void
fft_specialized(FP32Complex *list) {
    int even_idx, odd_idx, n = 2, lshift = MAX_LOG2_NUM_POINTS-1;
    FP32Complex exp_val, tw_val, even_val, odd_val;

    opt_bit_reverse(list, NUM_POINTS);

    while (n <= NUM_POINTS) {
        for (int i = 0; i < NUM_POINTS; i += n) {
            for (int k = 0; k < n/2; k++) {
                even_idx = i+k;
                odd_idx  = even_idx + n/2;

                exp_val = FP32Complex(opt_fft_cosf(k << lshift),
                                      opt_fft_sinf(k << lshift));

                even_val = list[even_idx];
                odd_val  = list[odd_idx];

                tw_val = exp_val*odd_val;

                list[even_idx] = even_val + tw_val;
                list[odd_idx]  = even_val - tw_val;
            }
        }
        n = n * 2;
        lshift--;
    }
}

/*******************************************************************************
 * Custom sinf/cosf utilities
*******************************************************************************/
// NOTE: the custom sinf/cosf functions should only be used for input sizes
// other than 128 and 256.

#ifdef CUSTOM_SINCOS
// https://stackoverflow.com/a/64063765

/* Argument reduction for trigonometric functions that reduces the argument
   to the interval [-PI/4, +PI/4] and also returns the quadrant. It returns 
   -0.0f for an input of -0.0f 
*/

// Force constants into DMEM to reduce remote DRAM loads
float trig_red_F0    = 6.36619747e-1f;
float trig_red_F1    = 1.57078552e+00f;
float trig_red_F2    = 1.08043314e-05f;
float trig_red_F3    = 2.56334407e-12f;
float trig_red_DELTA = 1.25829120e+7f;

// PP: calculation of j should NOT be optimized and therefore we declare
// an optimization flag -O0 for this function. There is not much to optimize
// anyway.
inline float
__attribute__((optimize("O0")))
trig_red_f (float a, int *q)
{    
    float j, r;

    /* Cody-Waite style reduction. W. J. Cody and W. Waite, "Software Manual
       for the Elementary Functions", Prentice-Hall 1980
    */
    /* j = (a * 0x1.45f306p-1f + 0x1.8p+23f) - 0x1.8p+23f; // 6.36619747e-1, 1.25829120e+7 */
    /* r = a - j * 0x1.921f00p+00f; // 1.57078552e+00 // pio2_high */
    /* r = r - j * 0x1.6a8880p-17f; // 1.08043314e-05 // pio2_mid */
    /* r = r - j * 0x1.68c234p-39f; // 2.56334407e-12 // pio2_low */
    j = (a * trig_red_F0 + trig_red_DELTA) - trig_red_DELTA; // 6.36619747e-1, 1.25829120e+7
    r = a - j * trig_red_F1; // 1.57078552e+00 // pio2_high
    r = r - j * trig_red_F2; // 1.08043314e-05 // pio2_mid
    r = r - j * trig_red_F3; // 2.56334407e-12 // pio2_low
    *q = (int)j;

    return r;
}

/* Approximate sine on [-PI/4,+PI/4]. Maximum ulp error with USE_FMA = 0.64196
   Returns -0.0f for an argument of -0.0f
   Polynomial approximation based on T. Myklebust, "Computing accurate 
   Horner form approximations to special functions in finite precision
   arithmetic", http://arxiv.org/abs/1508.03211, retrieved on 8/29/2016
*/

// Force constants into DMEM to reduce remote DRAM loads
float sinf_F0 =  2.86567956e-6f;
float sinf_F1 = -1.98559923e-4f;
float sinf_F2 =  8.33338592e-3f;
float sinf_F3 = -1.66666672e-1f;

inline float
sinf_poly (float a, float s)
{
    float r, t;
    /* r =         0x1.80a000p-19f; //  2.86567956e-6 */
    /* r = r * s - 0x1.a0690cp-13f; // -1.98559923e-4 */
    /* r = r * s + 0x1.111182p-07f; //  8.33338592e-3 */
    /* r = r * s - 0x1.555556p-03f; // -1.66666672e-1 */
    r =         sinf_F0; //  2.86567956e-6
    r = r * s + sinf_F1; // -1.98559923e-4
    r = r * s + sinf_F2; //  8.33338592e-3
    r = r * s + sinf_F3; // -1.66666672e-1
    t = a * s + zerof; // ensure -0 is passed through
    r = r * t + a;
    return r;
}

/* Approximate cosine on [-PI/4,+PI/4]. Maximum ulp error with USE_FMA = 0.87444 */

// Force constants into DMEM to reduce remote DRAM loads
float cosf_F0 =  2.44677067e-5f;
float cosf_F1 = -1.38877297e-3f;
float cosf_F2 =  4.16666567e-2f;
float cosf_F3 = -5.00000000e-1f;
float cosf_F4 =  1.00000000e+0f;

inline float
cosf_poly (float s)
{
    float r;
    /* r =         0x1.9a8000p-16f; //  2.44677067e-5 */
    /* r = r * s - 0x1.6c0efap-10f; // -1.38877297e-3 */
    /* r = r * s + 0x1.555550p-05f; //  4.16666567e-2 */
    /* r = r * s - 0x1.000000p-01f; // -5.00000000e-1 */
    /* r = r * s + 0x1.000000p+00f; //  1.00000000e+0 */
    r =         cosf_F0; //  2.44677067e-5
    r = r * s + cosf_F1; // -1.38877297e-3
    r = r * s + cosf_F2; //  4.16666567e-2
    r = r * s + cosf_F3; // -5.00000000e-1
    r = r * s + cosf_F4; //  1.00000000e+0
    return r;
}

/* Map sine or cosine value based on quadrant */
inline float
sinf_cosf_core (float a, int i)
{
    float r, s;

    s = a * a;
    r = (i & 1) ? cosf_poly (s) : sinf_poly (a, s);
    if (i & 2) {
        r = zerof - r; // don't change "sign" of NaNs
    }
    return r;
}

/* maximum ulp error with USE_FMA = 1: 1.495098  */
float my_sinf (float a)
{
    float r, p;
    int i;

    /* a = a * zerof + a; // inf -> NaN */
    r = trig_red_f (a, &i);
    p = sinf_cosf_core (r, i);
    /* if (__bsg_id == 1) { */
    /*     bsg_printf("my_sinf(%08X) is %08X (i=%d, r=%08X)\n", *(int*)&a, *(int*)&p, i, *(int*)&r); */
    /*     asm volatile("": : :"memory"); */
    /* } */
    return p;
}

/* maximum ulp error with USE_FMA = 1: 1.493253 */
float my_cosf (float a)
{
    float r, p;
    int i;

    /* a = a * zerof + a; // inf -> NaN */
    r = trig_red_f (a, &i);
    p = sinf_cosf_core (r, i + 1);
    /* if (__bsg_id == 1) { */
    /*     bsg_printf("my_cosf(%08X) is %08X (i=%d, r=%08X)\n", *(int*)&a, *(int*)&p, i, *(int*)&r); */
    /*     asm volatile("": : :"memory"); */
    /* } */
    return p;
}
#endif // CUSTOM_SINCOS

/*******************************************************************************
 * Four-step method utilities
*******************************************************************************/

// In-place implementation
void
fft_generic(FP32Complex *list, int N) {
    int even_idx, odd_idx, n = 2;
    FP32Complex exp_val, tw_val, even_val, odd_val;

    opt_bit_reverse(list, N);

    while (n <= N) {
        for (int i = 0; i < N; i += n) {
            for (int k = 0; k < n/2; k++) {
                even_idx = i+k;
                odd_idx  = even_idx + n/2;

#ifdef CUSTOM_SINCOS
                float ref_cosf = my_cosf(minus_2pi*(float)(k)/(float)(n));
                float ref_sinf = my_sinf(minus_2pi*(float)(k)/(float)(n));
#else
                float ref_sinf = sinf(minus_2pi*float(k)/float(n));
                float ref_cosf = cosf(minus_2pi*float(k)/float(n));
#endif

                exp_val = FP32Complex(ref_cosf, ref_sinf);

                even_val = list[even_idx];
                odd_val  = list[odd_idx];

                tw_val = exp_val*odd_val;

                list[even_idx] = even_val + tw_val;
                list[odd_idx]  = even_val - tw_val;
            }
        }
        n = n * 2;
    }
}

inline void
load_fft_store(FP32Complex *lst,
               FP32Complex *out,
               FP32Complex *local_lst,
               int start,
               int stride,
               int local_point,
               int total_point,
               int scaling)
{
    // Strided load into DMEM
    opt_data_transfer_src_strided(local_lst, lst+start, stride, local_point);

    /* debug_print_complex_per_tile(local_lst, local_point, "Loaded into DMEM"); */

    // 256-point or 128-point FFT
    if (local_point == NUM_POINTS)
        fft_specialized(local_lst);
    else
        fft_generic(local_lst, local_point);

    /* debug_print_complex_per_tile(local_lst, local_point, "After uFFT"); */

    // Optional twiddle scaling
    // TODO: eliminate all remote loads due to newlib sinf/cosf
    if (scaling) {
        for (int c = 0; c < local_point; c++) {
            FP32Complex w;
#ifdef CUSTOM_SINCOS
            float ref_sinf = my_sinf(minus_2pi*float(start*c)/float(total_point));
            float ref_cosf = my_cosf(minus_2pi*float(start*c)/float(total_point));
#else
            float ref_sinf = sinf(minus_2pi*float(start*c)/float(total_point));
            float ref_cosf = cosf(minus_2pi*float(start*c)/float(total_point));
#endif
            w = FP32Complex(ref_cosf, ref_sinf);
            local_lst[c] = w*local_lst[c];
        }
        /* debug_print_complex_per_tile(local_lst, local_point, "After scaling"); */
    }

    // Strided store into DRAM
    opt_data_transfer_dst_strided(out+start, local_lst, stride, local_point);
}

__attribute__ ((always_inline)) void
load_fft_store_no_twiddle(FP32Complex *lst,
                          FP32Complex *out,
                          FP32Complex *local_lst,
                          float bsg_attr_remote * bsg_attr_noalias tw,
                          //FP32Complex *tw,
                          int start,
                          int stride,
                          int local_point,
                          int total_point,
                          int scaling)
{
    // Strided load into DMEM
    opt_data_transfer_src_strided(local_lst, lst+start, stride, local_point);

    /* debug_print_complex(local_lst, local_point, "Loaded into DMEM"); */

    // 256-point or 128-point FFT
    if (local_point == NUM_POINTS)
        fft_specialized(local_lst);
    else
        fft_generic(local_lst, local_point);

    /* debug_print_complex(local_lst, local_point, "After uFFT"); */

    // Optional twiddle scaling
    if (scaling) {
            tw = tw+(sizeof(FP32Complex)/sizeof(float))*local_point*start;
            float buf[(sizeof(FP32Complex)/sizeof(float)) * UNROLL];
            FP32Complex *_tw = reinterpret_cast<FP32Complex *>(&buf);
    
            for (int c = 0; c < local_point; c+=UNROLL, tw+=(sizeof(FP32Complex)/sizeof(float))*UNROLL) {
                    bsg_unroll(8)
                    for(int u = 0; u < (UNROLL * (sizeof(FP32Complex)/sizeof(float))); ++u){
                            buf[u] = tw[u];
                    }
                    bsg_unroll(8)
                    for(int u = 0; u < UNROLL; ++u){
                            FP32Complex w = _tw[u];
                            local_lst[c + u] = w * local_lst[c + u];
                    }
            }
    }
    /*
    if (scaling) {
        tw = tw+local_point*start;
        for (int c = 0; c < local_point; c++, tw++) {
            FP32Complex w = *tw;
            local_lst[c] = w*local_lst[c];
            }*/
        /* debug_print_complex(local_lst, local_point, "After scaling"); */
    //}

    // Strided store into DRAM
    opt_data_transfer_dst_strided(out+start, local_lst, stride, local_point);
}

__attribute__ ((always_inline)) void
load_fft_scale_no_twiddle(FP32Complex *lst,
                          FP32Complex *local_lst,
                          float bsg_attr_remote * bsg_attr_noalias tw,
                          int start,
                          int stride,
                          int local_point,
                          int total_point)
{
    // Strided load into DMEM
    opt_data_transfer_src_strided(local_lst, lst+start, stride, local_point);

    /* debug_print_complex(local_lst, local_point, "Loaded into DMEM"); */

    // 256-point or 128-point FFT
    if (local_point == NUM_POINTS)
        fft_specialized(local_lst);
    else
        fft_generic(local_lst, local_point);

    /* debug_print_complex(local_lst, local_point, "After uFFT"); */

    // Twiddle scaling
    tw = tw+(sizeof(FP32Complex)/sizeof(float))*local_point*start;
    float buf[(sizeof(FP32Complex)/sizeof(float)) * UNROLL];
    FP32Complex *_tw = reinterpret_cast<FP32Complex *>(&buf);
    
    for (int c = 0; c < local_point; c+=UNROLL, tw+=(sizeof(FP32Complex)/sizeof(float))*UNROLL) {
            bsg_unroll(8)
            for(int u = 0; u < (UNROLL * (sizeof(FP32Complex)/sizeof(float))); ++u){
                    buf[u] = tw[u];
            }
            bsg_unroll(8)
            for(int u = 0; u < UNROLL; ++u){
                    FP32Complex w = _tw[u];
                    local_lst[c + u] = w * local_lst[c + u];
            }
    }
    
    /*
    for (int c = 0; c < local_point; c++, tw++) {
        FP32Complex w = *tw;
        local_lst[c] = w*local_lst[c];
    }
    */

    /* debug_print_complex(local_lst, local_point, "After scaling"); */
}

inline void
fft_store(FP32Complex *local_lst,
          FP32Complex *out,
          int start,
          int stride,
          int local_point,
          int total_point)
{
    /* debug_print_complex(local_lst, local_point, "Loaded into DMEM"); */

    // 256-point or 128-point FFT
    if (local_point == NUM_POINTS)
        fft_specialized(local_lst);
    else
        fft_generic(local_lst, local_point);

    /* debug_print_complex(local_lst, local_point, "After uFFT"); */

    // Strided store into DRAM
    opt_data_transfer_dst_strided(out+start, local_lst, stride, local_point);
}

// Unoptimized vector transposition: simply do remote loads and stores
// NOTE: this in-place algorithm is trivial because lst is a square matrix.
// In-place transposition for non-square matrices is possible but non-trivial.
inline void
square_transpose(FP32Complex *lst, int size) {
    FP32Complex tmp;
    // TODO: Increase unroll factoor
    for (int i = 0; i < size; i++)
        for (int j = i+1; j < size; j++) {
            tmp = lst[i+j*size];
            lst[i+j*size] = lst[j+i*size];
            lst[j+i*size] = tmp;
        }
}

// Only works for size 128 or 256!
inline void
opt_square_transpose(FP32Complex *lst, int size) {
    FP32Complex tmp;
    int i = __bsg_id;
    if (size == 128) {
        if (i >= 64) {
            for (int j = 0; j < 64; j++) {
                tmp = lst[i+j*size];
                lst[i+j*size] = lst[j+i*size];
                lst[j+i*size] = tmp;
            }
        } else {
            for (int j = i+1; j < 64; j++) {
                tmp = lst[i+j*size];
                lst[i+j*size] = lst[j+i*size];
                lst[j+i*size] = tmp;
            }
            for (int j = size-i; j < 128; j++) {
                int ii = size-1-i;
                tmp = lst[ii+j*size];
                lst[ii+j*size] = lst[j+ii*size];
                lst[j+ii*size] = tmp;
            }
        }
    } else {
        for (int j = i+1; j < size; j++) {
            tmp = lst[i+j*size];
            lst[i+j*size] = lst[j+i*size];
            lst[j+i*size] = tmp;
        }
        for (int j = size-i; j < size; j++) {
            int ii = size-1-i;
            tmp = lst[ii+j*size];
            lst[ii+j*size] = lst[j+ii*size];
            lst[j+ii*size] = tmp;
        }
    }
}

/*******************************************************************************
 * Tile-group shared memory utilities
*******************************************************************************/

typedef FP32Complex *bsg_remote_complex_ptr;

#define bsg_remote_complex(x, y, local_addr) \
    ((bsg_remote_complex_ptr)                \
        (                                    \
            (1 << REMOTE_PREFIX_SHIFT)       \
            | ((y) << REMOTE_Y_CORD_SHIFT)   \
            | ((x) << REMOTE_X_CORD_SHIFT)   \
            | ((int) (local_addr))           \
        )                                    \
    )

#define bsg_complex_store(x,y,local_addr,val)         \
    do {                                              \
        *(bsg_remote_complex((x),(y),(local_addr))) = \
            (FP32Complex) (val);                      \
    } while (0)

#define bsg_complex_load(x,y,local_addr,val)              \
    do {                                                  \
        val =                                             \
            *(bsg_remote_complex((x),(y),(local_addr)));  \
    } while (0)

// Out-of-place transposition using tile-group shared memory
// Only works for 128 tiles and 128x128-point FFT
inline void
tg_mem_square_transpose_oop(FP32Complex *dst, FP32Complex *src, int size) {
    FP32Complex *addr = dst+__bsg_id;
    for (int x = 0; x < bsg_tiles_X; x++)
        for (int y = 0; y < bsg_tiles_Y; y++) {
            bsg_complex_store(x, y, addr, *src);
            src++;
        }
}

// In-place transposition using tile-group shared memory
// Only works for 128 tiles and 128x128-point FFT
inline void
tg_mem_square_transpose_inp(FP32Complex *src, int size) {
    FP32Complex remote1, remote2;
    FP32Complex *addr = src;
    int i  = __bsg_id;
    int ix = __bsg_id & 0xF;
    int iy = __bsg_id >> 4;
    int j  = 0;
    int jx = 0;
    int jy = 0;

    if (i >= 64) {
        addr = src;
        for (j = 0; j < 64; j++) {
            jx = j & 0xF;
            jy = j >> 4;
            bsg_complex_load(jx, jy, src+i, remote1);
            bsg_complex_store(jx, jy, src+i, *addr);
            *addr = remote1;
            addr++;
        }
    } else {
        addr = src+(__bsg_id+1);
        for (j = __bsg_id+1; j < 64; j++) {
            jx = j & 0xF;
            jy = j >> 4;
            bsg_complex_load(jx, jy, src+i, remote1);
            bsg_complex_store(jx, jy, src+i, *addr);
            *addr = remote1;
            addr++;
        }
        i  = 127-__bsg_id;
        ix = i & 0xF;
        iy = i >> 4;
        addr = src+(128-__bsg_id);
        for (j = 128-__bsg_id; j < 128; j++) {
            jx = j & 0xF;
            jy = j >> 4;
            bsg_complex_load(jx, jy, src+i, remote1);
            bsg_complex_load(ix, iy, addr, remote2);
            bsg_complex_store(jx, jy, src+i, remote2);
            bsg_complex_store(ix, iy, addr, remote1);
            addr++;
        }
    }
}
