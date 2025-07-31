#ifndef PNGRAM_THREAD_HASH_COUNTER_HPP
#define PNGRAM_THREAD_HASH_COUNTER_HPP

#include <bitset>
#include <atomic>
#include "counts_array.hpp"

class ThreadHashCounter
{
 public:
  ThreadHashCounter();

  inline void set(const unsigned char* bytes);
  inline void clear();
  inline void flush(std::atomic_uint32_t* gram3_counts);
  inline void flush(CountsArray<>& countsArray);

  // 2MB
  alignas(64) uint64_t bits[262144];

  // Notes on things that do NOT help:
  //
  // - a prefetch buffer for gram3Counts... of any size
  // - skipping parts of the bits array...
  // - using uint16_ts instead of uint64_ts... that was like a 40% speedup
};

#include "thread_hash_counter_impl.hpp"

#endif
