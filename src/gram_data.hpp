#ifndef PNGRAM_GRAM_DATA_HPP
#define PNGRAM_GRAM_DATA_HPP

#include <atomic>
#include "alloc.hpp"

class GramData
{
 public:
  // Allow work on two files at a time.

  // Flush status:
  //    >= 3: work still in progress
  //     = 2: ready to flush, unclaimed
  //     = 1: flushing, claimed
  //     = 0: flush finished, ready for clear and new file
  std::atomic_uint8_t flushStatus1;
  std::atomic_uint8_t flushStatus2;

  std::atomic_uint64_t fileId1;
  std::atomic_uint64_t fileId2;

  constexpr static const size_t TotalAddressCount = (1 << 24);
  constexpr static const size_t TotalByteSize     = (TotalByteSize / 8);
  constexpr static const size_t ChunkAddressCount = (1 << 18);
  constexpr static const size_t ChunkBytes        = (ChunkAddressCount / 8);

  // Each thread should grab a 32k chunk of bits at a time;
  // preferably not switching unless really necessary.
  std::atomic_uint8_t bits1[TotalByteSize];
  std::atomic_uint8_t bits2[TotalByteSize];

  // This is the global array that should be flushed to.
  std::atomic_uint32_t* gram3Counts;
  alloc_mem_state gram3CountsMemState;

  inline GramData();
  inline ~GramData();
};

#include "gram_data_impl.hpp"

#endif
