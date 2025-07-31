#ifndef PNGRAM_PREFIX_MULTI_THREAD_HASH_COUNTER_HPP
#define PNGRAM_PREFIX_MULTI_THREAD_HASH_COUNTER_HPP

#include <bitset>
#include <atomic>
#include "counts_array.hpp"
#include "alloc.hpp"

class PrefixMultiThreadHashCounter
{
 public:
  PrefixMultiThreadHashCounter(const size_t elem, // number of elements we are counting
                               const PackedByteTrie<uint32_t>* prefixTrie,
                               const size_t prefixLen);
  ~PrefixMultiThreadHashCounter();

  inline void set(const unsigned char* bytes);
  inline void clear();

  template<bool FixedSize>
  inline void flush(CountsArray<FixedSize>& countsArray);

  template<bool FixedSize>
  inline void forceFlush(CountsArray<FixedSize>& countsArray);

  // 2MB
  alignas(64) uint64_t* bits;
  alloc_mem_state bitsMemState;
  size_t bitsIndex;
  const PackedByteTrie<uint32_t>* prefixTrie;
  const size_t prefixLen;
  const size_t bitsetLen;

  // Notes on things that do NOT help:
  //
  // - a prefetch buffer for gram3Counts... of any size
  // - skipping parts of the bits array...
  // - using uint16_ts instead of uint64_ts... that was like a 40% speedup
};

#include "prefix_multi_thread_hash_counter_impl.hpp"

#endif
