#pragma once

#include <string.h>
#include <string>

#ifndef CACHE_START_TYPE
#define CACHE_START_TYPE "ruchy"
#endif
#define xstr(x) str(x)
#define str(x) #x
#ifndef CREDIT_ALLOCATION
#define CREDIT_ALLOCATION 32
#endif

uint32_t get_credit_alloc(uint32_t y,uint32_t x){
        uint32_t allocs[8][16];
        if (strcmp(xstr(CREDIT_ALLOCATION),"doubledist")==0){
                uint32_t allocs[8][16] = {{30, 28, 28, 28, 26, 26, 26, 24, 24, 26, 26, 26, 28, 28, 28, 30},
                                          {28, 26, 26, 26, 24, 24, 24, 22, 22, 24, 24, 24, 26, 26, 26, 28},
                                          {26, 24, 24, 24, 22, 22, 22, 20, 20, 22, 22, 22, 24, 24, 24, 26},
                                          {24, 22, 22, 22, 20, 20, 20, 18, 18, 20, 20, 20, 22, 22, 22, 24},
                                          {24, 22, 22, 22, 20, 20, 20, 18, 18, 20, 20, 20, 22, 22, 22, 24},
                                          {26, 24, 24, 24, 22, 22, 22, 20, 20, 22, 22, 22, 24, 24, 24, 26},
                                          {28, 26, 26, 26, 24, 24, 24, 22, 22, 24, 24, 24, 26, 26, 26, 28},
                                          {30, 28, 28, 28, 26, 26, 26, 24, 24, 26, 26, 26, 28, 28, 28, 30}};
                return allocs[y][x];
        }else if (strcmp(xstr(CREDIT_ALLOCATION),"normdist")==0){
                uint32_t allocs[8][16] = {{12, 11, 11, 11, 10, 10, 10,  9,  9, 10, 10, 10, 11, 11, 11, 12},
                                          {11, 10, 10, 10,  9,  9,  9,  8,  8,  9,  9,  9, 10, 10, 10, 11},
                                          {10,  9,  9,  9,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9, 10},
                                          { 9,  8,  8,  8,  8,  8,  8,  7,  7,  8,  8,  8,  8,  8,  8,  9},
                                          { 9,  8,  8,  8,  8,  8,  8,  7,  7,  8,  8,  8,  8,  8,  8,  9},
                                          {10,  9,  9,  9,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9, 10},
                                          {11, 10, 10, 10,  9,  9,  9,  8,  8,  9,  9,  9, 10, 10, 10, 11},
                                          {12, 11, 11, 11, 10, 10, 10,  9,  9, 10, 10, 10, 11, 11, 11, 12}};
                return allocs[y][x];
        } else {
                uint32_t val = std::stoi(xstr(CREDIT_ALLOCATION));
                uint32_t allocs[8][16] = {{val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val}};
                return allocs[y][x];
        }
         
}

// cache_start_index(y_i-origin.y,x_i-origin.x)

int cache_start_index(int y,int x)
{
  if (strcmp(CACHE_START_TYPE,"ruchy")==0)
    return (y << 2) + ((0x24924924 >> (x << 1)) & 3);
  else
   if (strcmp(CACHE_START_TYPE,"ruchydeux")==0)
     return (y << 2) + (x & 3);
   else
   if (strcmp(CACHE_START_TYPE,"ruchytrois")==0)
     return (y << 2) + (x & 2);
   else
    if (strcmp(CACHE_START_TYPE,"xstripe")==0)
      return ((x << 1) + (y&1));
    else
    if (strcmp(CACHE_START_TYPE,"ystripe")==0)
      return (y << 2);
    else
      if (strcmp(CACHE_START_TYPE,"zero")==0)
	return 0;
      else
	return 0;
}

