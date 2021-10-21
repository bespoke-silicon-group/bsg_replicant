// BSG_TILE_GROUP_X_DIM and BSG_TILE_GROUP_Y_DIM must be defined
// before bsg_manycore.h and bsg_tile_group_barrier.h are
// included.
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#define BSG_TILE_GROUP_LOG_Y_DIM ((int)(log2(BSG_TILE_GROUP_Y_DIM)))
#define BSG_TILE_GROUP_LOG_X_DIM ((int)(log2(BSG_TILE_GROUP_X_DIM)))
#define MAKE_MASK(WIDTH) ((1UL << (WIDTH)) - 1UL)
#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>
#include <math.h>
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> g_barrier;

#define GROUP_EPA_WIDTH      18
#define GROUP_X_CORD_WIDTH   5
#define GROUP_Y_CORD_WIDTH   5
#define GROUP_X_CORD_SHIFT   (GROUP_EPA_WIDTH)
#define GROUP_Y_CORD_SHIFT   (GROUP_X_CORD_SHIFT+GROUP_X_CORD_WIDTH)
#define GROUP_PREFIX_SHIFT   (GROUP_Y_CORD_SHIFT+GROUP_Y_CORD_WIDTH + 1)

template<unsigned int TG_X, unsigned int S_X, unsigned int TG_Y, unsigned int S_Y, typename T>
class bsg_tile_group_strider{
        static const unsigned int Y_STRIDE = (1 << GROUP_Y_CORD_SHIFT);
        static const unsigned int X_STRIDE = (1 << GROUP_X_CORD_SHIFT);
        static const unsigned int X_MASK = ~(MAKE_MASK(GROUP_Y_CORD_WIDTH - (unsigned int)(log2(TG_Y))) << ((unsigned int)(log2(TG_Y)) + GROUP_Y_CORD_SHIFT));
        static const unsigned int Y_MASK = ~(MAKE_MASK(GROUP_X_CORD_WIDTH - (unsigned int)(log2(TG_X))) << ((unsigned int)(log2(TG_X)) + GROUP_X_CORD_SHIFT));

protected:
public:
        T *ptr;
        bsg_tile_group_strider(T &p){
                ptr =(T*)( ((1 << GROUP_PREFIX_SHIFT)
                            | (__bsg_y << GROUP_Y_CORD_SHIFT)
                            | (__bsg_x << GROUP_X_CORD_SHIFT)
                            | ((unsigned int) &p)));
        }

        T* stride(){
                if(S_X == 0){
                        return ptr = (T*)(((unsigned int) ptr + Y_STRIDE) & Y_MASK);
                } else if(S_Y == 0){
                        return ptr = (T*)(((unsigned int) ptr + X_STRIDE) & X_MASK);
                } else {
                        return ptr = (T*)(((((unsigned int) ptr + X_STRIDE) & X_MASK) + Y_STRIDE) & Y_MASK);
                }
        }

};

/*
template<unsigned int TG_X, unsigned int S_X, unsigned int TG_Y, unsigned int S_Y, typename T>
class bsg_tile_group_strider<TG_X, S_X, TG_Y, 0, T>{
protected:
public:
        T *ptr;
        bsg_tile_group_strider(T &p){
                ptr =(T*)( ((1 << GROUP_PREFIX_SHIFT)
                            | (__bsg_y << GROUP_Y_CORD_SHIFT)
                            | (__bsg_x << GROUP_X_CORD_SHIFT)
                            | ((unsigned int) &p)));
        }

        T* stride(){
        }

};
template<unsigned int TG_X, unsigned int S_X, unsigned int TG_Y, unsigned int S_Y, typename T>
class bsg_tile_group_strider<TG_X, 0, TG_Y, S_Y, T>{
protected:
public:
        T* stride(){
                return ptr = (T*)(((unsigned int) ptr + Y_STRIDE) & Y_MASK);
        }

        };*/

extern "C" int kernel_group_stride(int *nx, int *ny){
        volatile int *self = bsg_tile_group_remote_ptr(int, __bsg_x, __bsg_y, (int*) 0);
        unsigned int y_stride = (1 << GROUP_Y_CORD_SHIFT);
        unsigned int x_stride = (1 << GROUP_X_CORD_SHIFT);
        unsigned int y_mask = ~(MAKE_MASK(GROUP_Y_CORD_WIDTH - BSG_TILE_GROUP_LOG_Y_DIM) << (BSG_TILE_GROUP_LOG_Y_DIM + GROUP_Y_CORD_SHIFT));
        unsigned int x_mask = ~(MAKE_MASK(GROUP_X_CORD_WIDTH - BSG_TILE_GROUP_LOG_X_DIM) << (BSG_TILE_GROUP_LOG_X_DIM + GROUP_X_CORD_SHIFT));
        //bsg_print_hexadecimal((unsigned int)bsg_tile_group_remote_ptr(int, (__bsg_x + 1) % BSG_TILE_GROUP_X_DIM, __bsg_y, &__bsg_x));
        //bsg_print_hexadecimal(((unsigned int)self + x_stride + (unsigned int) &__bsg_x) & x_mask);
        bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 1, BSG_TILE_GROUP_Y_DIM, 0, int> stride_x(__bsg_x);
        bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 1, int> stride_y(__bsg_y);
        nx[__bsg_y * BSG_TILE_GROUP_X_DIM + __bsg_x] = *stride_x.stride();
        ny[__bsg_y * BSG_TILE_GROUP_X_DIM + __bsg_x] = *stride_y.stride();
        g_barrier.sync();        
        return 0;
}

