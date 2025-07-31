#ifndef PNGRAM_POOL_THREAD_HASH_COUNTER_HPP
#define PNGRAM_POOL_THREAD_HASH_COUNTER_HPP

#include <bitset>
#include <atomic>
#include "counts_array.hpp"

template<size_t C>
class PoolThreadHashCounter
{
 public:
  PoolThreadHashCounter(uint64_t** bitsets,
                        std::atomic_uint8_t* bitsetStatuses);

  inline void claim();
  inline void set(const unsigned char* bytes);
  inline void clear();
  inline void flush(CountsArray& countsArray);

  inline void forceFlush(std::atomic_uint32_t* gram3_counts);
  inline void forceFlush(CountsArray& countsArray);

  uint64_t** bitsets;
  std::atomic_uint8_t* bitsetStatuses;
  size_t bitsIndex;
  size_t waitingForBitset;
  size_t flushes;

  // statuses:
  //  0: available for use, all zeros
  //  1: being filled
  //  2: needs to be flushed
  //  3: flushing
  //  4: finished
};

#include "pool_thread_hash_counter_impl.hpp"

#endif
