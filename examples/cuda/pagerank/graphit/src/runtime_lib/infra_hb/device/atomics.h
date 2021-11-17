#pragma once
#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_manycore_atomic.h>
#include <atomic>

__attribute__((section(".dram"))) std::atomic<int> * __restrict locks; //this is initialized in host program to be 1024 (16 * 64) in len

void acquire_lock(unsigned i)
{
  int l = 1;
  do{ l = locks[i].exchange(1, std::memory_order_acquire); } while (l == 1);
}

void release_lock(unsigned i)
{
  locks[i].store(0, std::memory_order_release);
}

template <typename T>
bool compare_and_swap(T& x, const T &old_val, const T &new_val)
{
  unsigned addr = (unsigned) &x;
  unsigned mask = (unsigned) 0x3FF; //assuming 1024 locks
  unsigned ind = (addr >> 2) & mask;
  acquire_lock(ind);
  T v = x;
  bool r = false;
  if(x == old_val) {
    r = true;
    x = new_val;
  }
  release_lock(ind);
  return r;
}

template <typename T>
T fetch_and_add(T& x, T inc)
{
  T newV, oldV;
  do{oldV = x; newV = oldV + inc;}
  while(!compare_and_swap<T>(x, oldV, newV));
  return oldV;
}

template <typename T>
bool writeMin(T &a, T b) {
  T c; bool r = 0;
  if(a < b)
    return false;
  do {c = a;}
  while(c > b && !(r = compare_and_swap<T>(a,c,b)));
  return r;
}

template <typename T>
bool writeMax(T &a, T b) {
  T c; bool r = 0;
  if(a >= b)
    return false;
  do {c = a;}
  while(c < b && !(r = compare_and_swap<T>(a,c,b)));
  return r;
}

template <typename T>
void writeAdd(T &a, T b) {
  T temp = fetch_and_add<T>(a, b);
}
