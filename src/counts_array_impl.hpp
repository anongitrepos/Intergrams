#ifndef PNGRAM_COUNTS_ARRAY_IMPL_HPP
#define PNGRAM_COUNTS_ARRAY_IMPL_HPP

#include "counts_array.hpp"
#include <cstring>
#include <fstream>

template<bool FixedSize>
inline CountsArray<FixedSize>::CountsArray() :
    prefixTrie(nullptr)
{
  if (FixedSize == false)
    throw std::runtime_error("Must initialize CountsArray with a size!");

  alloc_hugepage<u32_512>(counts, countsMemState, 1048576, "n-gramming");
  mutexes = new std::mutex[8192];

  for (size_t i = 0; i < 1048576; ++i)
    for (size_t j = 0; j < 16; ++j)
      counts[i][j] = uint32_t(0);
}

template<bool FixedSize>
inline CountsArray<FixedSize>::CountsArray(const size_t sizeIn,
                                           const PackedByteTrie<uint32_t>* prefixTrieIn) :
    size(sizeIn / 16),
    prefixTrie(prefixTrieIn)
{
  if (FixedSize == true)
    throw std::runtime_error("Must initialize CountsArray without a size!");

  alloc_hugepage<u32_512>(counts, countsMemState, size, "n-gramming");
  mutexes = new std::mutex[(size + 127) / 128];

  for (size_t i = 0; i < size; ++i)
    for (size_t j = 0; j < 16; ++j)
      counts[i][j] = uint32_t(0);
}

template<bool FixedSize>
inline CountsArray<FixedSize>::~CountsArray()
{
  delete[] mutexes;
  free_hugepage<u32_512>(counts, countsMemState, FixedSize ? 1048576 : size);
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Increment(const size_t index,
                                              const uint64_t bits)
{
  // required because each u32_512 is not atomic
  const size_t blockIndex = index * 4;
  const size_t mutexIndex = blockIndex / 128;
  const std::lock_guard lock(mutexes[mutexIndex]);

  // After tuning, this prefetch does make a bit of a difference, and the right
  // lookahead appears to be 1 blocks.  ...at least when we have 64 threads.
  __builtin_prefetch(&counts[blockIndex + 5], 1, 3);

  constexpr const u32_512 mask = { 0x0001, 0x0002, 0x0004, 0x0008,
                                   0x0010, 0x0020, 0x0040, 0x0080,
                                   0x0100, 0x0200, 0x0400, 0x0800,
                                   0x1000, 0x2000, 0x4000, 0x8000 };
  constexpr const u32_512 shift = { 0x0000, 0x0001, 0x0002, 0x0003,
                                    0x0004, 0x0005, 0x0006, 0x0007,
                                    0x0008, 0x0009, 0x000a, 0x000b,
                                    0x000c, 0x000d, 0x000e, 0x000f };

  uint32_t bits32_1 = (uint32_t(bits) & 0x0000FFFF);
  uint32_t bits32_2 = ((uint32_t(bits) & 0xFFFF0000) >> 16);
  uint32_t bits32_3 = (uint64_t(bits >> 32) & 0x0000FFFF);
  uint32_t bits32_4 = ((uint64_t(bits >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr1 = { bits32_1, bits32_1, bits32_1, bits32_1,
                    bits32_1, bits32_1, bits32_1, bits32_1,
                    bits32_1, bits32_1, bits32_1, bits32_1,
                    bits32_1, bits32_1, bits32_1, bits32_1 };
  u32_512 incr2 = { bits32_2, bits32_2, bits32_2, bits32_2,
                    bits32_2, bits32_2, bits32_2, bits32_2,
                    bits32_2, bits32_2, bits32_2, bits32_2,
                    bits32_2, bits32_2, bits32_2, bits32_2 };
  u32_512 incr3 = { bits32_3, bits32_3, bits32_3, bits32_3,
                    bits32_3, bits32_3, bits32_3, bits32_3,
                    bits32_3, bits32_3, bits32_3, bits32_3,
                    bits32_3, bits32_3, bits32_3, bits32_3 };
  u32_512 incr4 = { bits32_4, bits32_4, bits32_4, bits32_4,
                    bits32_4, bits32_4, bits32_4, bits32_4,
                    bits32_4, bits32_4, bits32_4, bits32_4,
                    bits32_4, bits32_4, bits32_4, bits32_4 };

  counts[blockIndex]     += ((incr1 & mask) >> shift);
  counts[blockIndex + 1] += ((incr2 & mask) >> shift);
  counts[blockIndex + 2] += ((incr3 & mask) >> shift);
  counts[blockIndex + 3] += ((incr4 & mask) >> shift);
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Increment(const size_t index,
                                              const uint64_t bits1,
                                              const uint64_t bits2)
{
  // required because each u32_512 is not atomic
  const size_t blockIndex = index * 4;
  const size_t mutexIndex = blockIndex / 128;
  const std::lock_guard lock(mutexes[mutexIndex]);

  // After tuning, this prefetch does make a bit of a difference, and the right
  // lookahead appears to be 1 blocks.  ...at least when we have 64 threads.
  //__builtin_prefetch(&counts[blockIndex + 16], 1, 3);

  constexpr const u32_512 mask = { 0x0001, 0x0002, 0x0004, 0x0008,
                                   0x0010, 0x0020, 0x0040, 0x0080,
                                   0x0100, 0x0200, 0x0400, 0x0800,
                                   0x1000, 0x2000, 0x4000, 0x8000 };
  constexpr const u32_512 shift = { 0x0000, 0x0001, 0x0002, 0x0003,
                                    0x0004, 0x0005, 0x0006, 0x0007,
                                    0x0008, 0x0009, 0x000a, 0x000b,
                                    0x000c, 0x000d, 0x000e, 0x000f };

  uint32_t bits32_1_1 = (uint32_t(bits1) & 0x0000FFFF);
  uint32_t bits32_1_2 = ((uint32_t(bits1) & 0xFFFF0000) >> 16);
  uint32_t bits32_1_3 = (uint64_t(bits1 >> 32) & 0x0000FFFF);
  uint32_t bits32_1_4 = ((uint64_t(bits1 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr1_1 = { bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1 };
  u32_512 incr1_2 = { bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2 };
  u32_512 incr1_3 = { bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3 };
  u32_512 incr1_4 = { bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4 };

  uint32_t bits32_2_1 = (uint32_t(bits2) & 0x0000FFFF);
  uint32_t bits32_2_2 = ((uint32_t(bits2) & 0xFFFF0000) >> 16);
  uint32_t bits32_2_3 = (uint64_t(bits2 >> 32) & 0x0000FFFF);
  uint32_t bits32_2_4 = ((uint64_t(bits2 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr2_1 = { bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1 };
  u32_512 incr2_2 = { bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2 };
  u32_512 incr2_3 = { bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3 };
  u32_512 incr2_4 = { bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4 };

  counts[blockIndex]     += ((incr1_1 & mask) >> shift) + ((incr2_1 & mask) >> shift);
  counts[blockIndex + 1] += ((incr1_2 & mask) >> shift) + ((incr2_2 & mask) >> shift);
  counts[blockIndex + 2] += ((incr1_3 & mask) >> shift) + ((incr2_3 & mask) >> shift);
  counts[blockIndex + 3] += ((incr1_4 & mask) >> shift) + ((incr2_4 & mask) >> shift);
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Increment(const size_t index,
                                              const uint64_t bits1,
                                              const uint64_t bits2,
                                              const uint64_t bits3)
{
  // required because each u32_512 is not atomic
  const size_t blockIndex = index * 4;
  const size_t mutexIndex = blockIndex / 128;
  const std::lock_guard lock(mutexes[mutexIndex]);

  // After tuning, this prefetch does make a bit of a difference, and the right
  // lookahead appears to be 1 blocks.  ...at least when we have 64 threads.
  //__builtin_prefetch(&counts[blockIndex + 16], 1, 3);

  constexpr const u32_512 mask = { 0x0001, 0x0002, 0x0004, 0x0008,
                                   0x0010, 0x0020, 0x0040, 0x0080,
                                   0x0100, 0x0200, 0x0400, 0x0800,
                                   0x1000, 0x2000, 0x4000, 0x8000 };
  constexpr const u32_512 shift = { 0x0000, 0x0001, 0x0002, 0x0003,
                                    0x0004, 0x0005, 0x0006, 0x0007,
                                    0x0008, 0x0009, 0x000a, 0x000b,
                                    0x000c, 0x000d, 0x000e, 0x000f };

  uint32_t bits32_1_1 = (uint32_t(bits1) & 0x0000FFFF);
  uint32_t bits32_1_2 = ((uint32_t(bits1) & 0xFFFF0000) >> 16);
  uint32_t bits32_1_3 = (uint64_t(bits1 >> 32) & 0x0000FFFF);
  uint32_t bits32_1_4 = ((uint64_t(bits1 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr1_1 = { bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1 };
  u32_512 incr1_2 = { bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2 };
  u32_512 incr1_3 = { bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3 };
  u32_512 incr1_4 = { bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4 };

  uint32_t bits32_2_1 = (uint32_t(bits2) & 0x0000FFFF);
  uint32_t bits32_2_2 = ((uint32_t(bits2) & 0xFFFF0000) >> 16);
  uint32_t bits32_2_3 = (uint64_t(bits2 >> 32) & 0x0000FFFF);
  uint32_t bits32_2_4 = ((uint64_t(bits2 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr2_1 = { bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1 };
  u32_512 incr2_2 = { bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2 };
  u32_512 incr2_3 = { bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3 };
  u32_512 incr2_4 = { bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4 };

  uint32_t bits32_3_1 = (uint32_t(bits3) & 0x0000FFFF);
  uint32_t bits32_3_2 = ((uint32_t(bits3) & 0xFFFF0000) >> 16);
  uint32_t bits32_3_3 = (uint64_t(bits3 >> 32) & 0x0000FFFF);
  uint32_t bits32_3_4 = ((uint64_t(bits3 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr3_1 = { bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1 };
  u32_512 incr3_2 = { bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2 };
  u32_512 incr3_3 = { bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3 };
  u32_512 incr3_4 = { bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4 };

  counts[blockIndex]     += ((incr1_1 & mask) >> shift) +
      ((incr2_1 & mask) >> shift) + ((incr3_1 & mask) >> shift);
  counts[blockIndex + 1] += ((incr1_2 & mask) >> shift) +
      ((incr2_2 & mask) >> shift) + ((incr3_2 & mask) >> shift);
  counts[blockIndex + 2] += ((incr1_3 & mask) >> shift) +
      ((incr2_3 & mask) >> shift) + ((incr3_3 & mask) >> shift);
  counts[blockIndex + 3] += ((incr1_4 & mask) >> shift) +
      ((incr2_4 & mask) >> shift) + ((incr3_4 & mask) >> shift);
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Increment(const size_t index,
                                              const uint64_t bits1,
                                              const uint64_t bits2,
                                              const uint64_t bits3,
                                              const uint64_t bits4)
{
  // required because each u32_512 is not atomic
  const size_t blockIndex = index * 4;
  const size_t mutexIndex = blockIndex / 128;
  const std::lock_guard lock(mutexes[mutexIndex]);

  // After tuning, this prefetch does make a bit of a difference, and the right
  // lookahead appears to be 1 blocks.  ...at least when we have 64 threads.
  //__builtin_prefetch(&counts[blockIndex + 16], 1, 3);

  constexpr const u32_512 mask = { 0x0001, 0x0002, 0x0004, 0x0008,
                                   0x0010, 0x0020, 0x0040, 0x0080,
                                   0x0100, 0x0200, 0x0400, 0x0800,
                                   0x1000, 0x2000, 0x4000, 0x8000 };
  constexpr const u32_512 shift = { 0x0000, 0x0001, 0x0002, 0x0003,
                                    0x0004, 0x0005, 0x0006, 0x0007,
                                    0x0008, 0x0009, 0x000a, 0x000b,
                                    0x000c, 0x000d, 0x000e, 0x000f };

  uint32_t bits32_1_1 = (uint32_t(bits1) & 0x0000FFFF);
  uint32_t bits32_1_2 = ((uint32_t(bits1) & 0xFFFF0000) >> 16);
  uint32_t bits32_1_3 = (uint64_t(bits1 >> 32) & 0x0000FFFF);
  uint32_t bits32_1_4 = ((uint64_t(bits1 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr1_1 = { bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1 };
  u32_512 incr1_2 = { bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2 };
  u32_512 incr1_3 = { bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3 };
  u32_512 incr1_4 = { bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4 };

  uint32_t bits32_2_1 = (uint32_t(bits2) & 0x0000FFFF);
  uint32_t bits32_2_2 = ((uint32_t(bits2) & 0xFFFF0000) >> 16);
  uint32_t bits32_2_3 = (uint64_t(bits2 >> 32) & 0x0000FFFF);
  uint32_t bits32_2_4 = ((uint64_t(bits2 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr2_1 = { bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1 };
  u32_512 incr2_2 = { bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2 };
  u32_512 incr2_3 = { bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3 };
  u32_512 incr2_4 = { bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4 };

  uint32_t bits32_3_1 = (uint32_t(bits3) & 0x0000FFFF);
  uint32_t bits32_3_2 = ((uint32_t(bits3) & 0xFFFF0000) >> 16);
  uint32_t bits32_3_3 = (uint64_t(bits3 >> 32) & 0x0000FFFF);
  uint32_t bits32_3_4 = ((uint64_t(bits3 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr3_1 = { bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1 };
  u32_512 incr3_2 = { bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2 };
  u32_512 incr3_3 = { bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3 };
  u32_512 incr3_4 = { bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4 };

  uint32_t bits32_4_1 = (uint32_t(bits4) & 0x0000FFFF);
  uint32_t bits32_4_2 = ((uint32_t(bits4) & 0xFFFF0000) >> 16);
  uint32_t bits32_4_3 = (uint64_t(bits4 >> 32) & 0x0000FFFF);
  uint32_t bits32_4_4 = ((uint64_t(bits4 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr4_1 = { bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1 };
  u32_512 incr4_2 = { bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2 };
  u32_512 incr4_3 = { bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3 };
  u32_512 incr4_4 = { bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4 };

  counts[blockIndex]     += ((incr1_1 & mask) >> shift) +
      ((incr2_1 & mask) >> shift) + ((incr3_1 & mask) >> shift) +
      ((incr4_1 & mask) >> shift);
  counts[blockIndex + 1] += ((incr1_2 & mask) >> shift) +
      ((incr2_2 & mask) >> shift) + ((incr3_2 & mask) >> shift) +
      ((incr4_2 & mask) >> shift);
  counts[blockIndex + 2] += ((incr1_3 & mask) >> shift) +
      ((incr2_3 & mask) >> shift) + ((incr3_3 & mask) >> shift) +
      ((incr4_3 & mask) >> shift);
  counts[blockIndex + 3] += ((incr1_4 & mask) >> shift) +
      ((incr2_4 & mask) >> shift) + ((incr3_4 & mask) >> shift) +
      ((incr4_4 & mask) >> shift);
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Increment(const size_t index,
                                              const uint64_t bits1,
                                              const uint64_t bits2,
                                              const uint64_t bits3,
                                              const uint64_t bits4,
                                              const uint64_t bits5,
                                              const uint64_t bits6,
                                              const uint64_t bits7,
                                              const uint64_t bits8)
{
  // required because each u32_512 is not atomic
  const size_t blockIndex = index * 4;
  const size_t mutexIndex = blockIndex / 128;
  const std::lock_guard lock(mutexes[mutexIndex]);

  // After tuning, this prefetch does make a bit of a difference, and the right
  // lookahead appears to be 1 blocks.  ...at least when we have 64 threads.
  //__builtin_prefetch(&counts[blockIndex + 16], 1, 3);

  constexpr const u32_512 mask = { 0x0001, 0x0002, 0x0004, 0x0008,
                                   0x0010, 0x0020, 0x0040, 0x0080,
                                   0x0100, 0x0200, 0x0400, 0x0800,
                                   0x1000, 0x2000, 0x4000, 0x8000 };
  constexpr const u32_512 shift = { 0x0000, 0x0001, 0x0002, 0x0003,
                                    0x0004, 0x0005, 0x0006, 0x0007,
                                    0x0008, 0x0009, 0x000a, 0x000b,
                                    0x000c, 0x000d, 0x000e, 0x000f };

  uint32_t bits32_1_1 = (uint32_t(bits1) & 0x0000FFFF);
  uint32_t bits32_1_2 = ((uint32_t(bits1) & 0xFFFF0000) >> 16);
  uint32_t bits32_1_3 = (uint64_t(bits1 >> 32) & 0x0000FFFF);
  uint32_t bits32_1_4 = ((uint64_t(bits1 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr1_1 = { bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1 };
  u32_512 incr1_2 = { bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2 };
  u32_512 incr1_3 = { bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3 };
  u32_512 incr1_4 = { bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4 };

  uint32_t bits32_2_1 = (uint32_t(bits2) & 0x0000FFFF);
  uint32_t bits32_2_2 = ((uint32_t(bits2) & 0xFFFF0000) >> 16);
  uint32_t bits32_2_3 = (uint64_t(bits2 >> 32) & 0x0000FFFF);
  uint32_t bits32_2_4 = ((uint64_t(bits2 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr2_1 = { bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1 };
  u32_512 incr2_2 = { bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2 };
  u32_512 incr2_3 = { bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3 };
  u32_512 incr2_4 = { bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4 };

  uint32_t bits32_3_1 = (uint32_t(bits3) & 0x0000FFFF);
  uint32_t bits32_3_2 = ((uint32_t(bits3) & 0xFFFF0000) >> 16);
  uint32_t bits32_3_3 = (uint64_t(bits3 >> 32) & 0x0000FFFF);
  uint32_t bits32_3_4 = ((uint64_t(bits3 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr3_1 = { bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1 };
  u32_512 incr3_2 = { bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2 };
  u32_512 incr3_3 = { bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3 };
  u32_512 incr3_4 = { bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4 };

  uint32_t bits32_4_1 = (uint32_t(bits4) & 0x0000FFFF);
  uint32_t bits32_4_2 = ((uint32_t(bits4) & 0xFFFF0000) >> 16);
  uint32_t bits32_4_3 = (uint64_t(bits4 >> 32) & 0x0000FFFF);
  uint32_t bits32_4_4 = ((uint64_t(bits4 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr4_1 = { bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1 };
  u32_512 incr4_2 = { bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2 };
  u32_512 incr4_3 = { bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3 };
  u32_512 incr4_4 = { bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4 };

  uint32_t bits32_5_1 = (uint32_t(bits5) & 0x0000FFFF);
  uint32_t bits32_5_2 = ((uint32_t(bits5) & 0xFFFF0000) >> 16);
  uint32_t bits32_5_3 = (uint64_t(bits5 >> 32) & 0x0000FFFF);
  uint32_t bits32_5_4 = ((uint64_t(bits5 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr5_1 = { bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1,
                      bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1,
                      bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1,
                      bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1 };
  u32_512 incr5_2 = { bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2,
                      bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2,
                      bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2,
                      bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2 };
  u32_512 incr5_3 = { bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3,
                      bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3,
                      bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3,
                      bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3 };
  u32_512 incr5_4 = { bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4,
                      bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4,
                      bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4,
                      bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4 };

  uint32_t bits32_6_1 = (uint32_t(bits6) & 0x0000FFFF);
  uint32_t bits32_6_2 = ((uint32_t(bits6) & 0xFFFF0000) >> 16);
  uint32_t bits32_6_3 = (uint64_t(bits6 >> 32) & 0x0000FFFF);
  uint32_t bits32_6_4 = ((uint64_t(bits6 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr6_1 = { bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1,
                      bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1,
                      bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1,
                      bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1 };
  u32_512 incr6_2 = { bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2,
                      bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2,
                      bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2,
                      bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2 };
  u32_512 incr6_3 = { bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3,
                      bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3,
                      bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3,
                      bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3 };
  u32_512 incr6_4 = { bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4,
                      bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4,
                      bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4,
                      bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4 };

  uint32_t bits32_7_1 = (uint32_t(bits7) & 0x0000FFFF);
  uint32_t bits32_7_2 = ((uint32_t(bits7) & 0xFFFF0000) >> 16);
  uint32_t bits32_7_3 = (uint64_t(bits7 >> 32) & 0x0000FFFF);
  uint32_t bits32_7_4 = ((uint64_t(bits7 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr7_1 = { bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1,
                      bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1,
                      bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1,
                      bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1 };
  u32_512 incr7_2 = { bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2,
                      bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2,
                      bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2,
                      bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2 };
  u32_512 incr7_3 = { bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3,
                      bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3,
                      bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3,
                      bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3 };
  u32_512 incr7_4 = { bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4,
                      bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4,
                      bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4,
                      bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4 };

  uint32_t bits32_8_1 = (uint32_t(bits8) & 0x0000FFFF);
  uint32_t bits32_8_2 = ((uint32_t(bits8) & 0xFFFF0000) >> 16);
  uint32_t bits32_8_3 = (uint64_t(bits8 >> 32) & 0x0000FFFF);
  uint32_t bits32_8_4 = ((uint64_t(bits8 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr8_1 = { bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1,
                      bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1,
                      bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1,
                      bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1 };
  u32_512 incr8_2 = { bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2,
                      bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2,
                      bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2,
                      bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2 };
  u32_512 incr8_3 = { bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3,
                      bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3,
                      bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3,
                      bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3 };
  u32_512 incr8_4 = { bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4,
                      bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4,
                      bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4,
                      bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4 };

  counts[blockIndex]     += ((incr1_1 & mask) >> shift) +
      ((incr2_1 & mask) >> shift) + ((incr3_1 & mask) >> shift) +
      ((incr4_1 & mask) >> shift) + ((incr5_1 & mask) >> shift) +
      ((incr6_1 & mask) >> shift) + ((incr7_1 & mask) >> shift) +
      ((incr8_1 & mask) >> shift);
  counts[blockIndex + 1] += ((incr1_2 & mask) >> shift) +
      ((incr2_2 & mask) >> shift) + ((incr3_2 & mask) >> shift) +
      ((incr4_2 & mask) >> shift) + ((incr5_2 & mask) >> shift) +
      ((incr6_2 & mask) >> shift) + ((incr7_2 & mask) >> shift) +
      ((incr8_2 & mask) >> shift);
  counts[blockIndex + 2] += ((incr1_3 & mask) >> shift) +
      ((incr2_3 & mask) >> shift) + ((incr3_3 & mask) >> shift) +
      ((incr4_3 & mask) >> shift) + ((incr5_3 & mask) >> shift) +
      ((incr6_3 & mask) >> shift) + ((incr7_3 & mask) >> shift) +
      ((incr8_3 & mask) >> shift);
  counts[blockIndex + 3] += ((incr1_4 & mask) >> shift) +
      ((incr2_4 & mask) >> shift) + ((incr3_4 & mask) >> shift) +
      ((incr4_4 & mask) >> shift) + ((incr5_4 & mask) >> shift) +
      ((incr6_4 & mask) >> shift) + ((incr7_4 & mask) >> shift) +
      ((incr8_4 & mask) >> shift);
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Increment(const size_t index,
                                              const uint64_t bits1,
                                              const uint64_t bits2,
                                              const uint64_t bits3,
                                              const uint64_t bits4,
                                              const uint64_t bits5,
                                              const uint64_t bits6,
                                              const uint64_t bits7,
                                              const uint64_t bits8,
                                              const uint64_t bits9,
                                              const uint64_t bits10,
                                              const uint64_t bits11,
                                              const uint64_t bits12,
                                              const uint64_t bits13,
                                              const uint64_t bits14,
                                              const uint64_t bits15,
                                              const uint64_t bits16)
{
  // required because each u32_512 is not atomic
  const size_t blockIndex = index * 4;
  const size_t mutexIndex = blockIndex / 128;
  const std::lock_guard lock(mutexes[mutexIndex]);

  // After tuning, this prefetch does make a bit of a difference, and the right
  // lookahead appears to be 1 blocks.  ...at least when we have 64 threads.
  //__builtin_prefetch(&counts[blockIndex + 16], 1, 3);

  constexpr const u32_512 mask = { 0x0001, 0x0002, 0x0004, 0x0008,
                                   0x0010, 0x0020, 0x0040, 0x0080,
                                   0x0100, 0x0200, 0x0400, 0x0800,
                                   0x1000, 0x2000, 0x4000, 0x8000 };
  constexpr const u32_512 shift = { 0x0000, 0x0001, 0x0002, 0x0003,
                                    0x0004, 0x0005, 0x0006, 0x0007,
                                    0x0008, 0x0009, 0x000a, 0x000b,
                                    0x000c, 0x000d, 0x000e, 0x000f };

  uint32_t bits32_1_1 = (uint32_t(bits1) & 0x0000FFFF);
  uint32_t bits32_1_2 = ((uint32_t(bits1) & 0xFFFF0000) >> 16);
  uint32_t bits32_1_3 = (uint64_t(bits1 >> 32) & 0x0000FFFF);
  uint32_t bits32_1_4 = ((uint64_t(bits1 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr1_1 = { bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1,
                      bits32_1_1, bits32_1_1, bits32_1_1, bits32_1_1 };
  u32_512 incr1_2 = { bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2,
                      bits32_1_2, bits32_1_2, bits32_1_2, bits32_1_2 };
  u32_512 incr1_3 = { bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3,
                      bits32_1_3, bits32_1_3, bits32_1_3, bits32_1_3 };
  u32_512 incr1_4 = { bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4,
                      bits32_1_4, bits32_1_4, bits32_1_4, bits32_1_4 };

  uint32_t bits32_2_1 = (uint32_t(bits2) & 0x0000FFFF);
  uint32_t bits32_2_2 = ((uint32_t(bits2) & 0xFFFF0000) >> 16);
  uint32_t bits32_2_3 = (uint64_t(bits2 >> 32) & 0x0000FFFF);
  uint32_t bits32_2_4 = ((uint64_t(bits2 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr2_1 = { bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1,
                      bits32_2_1, bits32_2_1, bits32_2_1, bits32_2_1 };
  u32_512 incr2_2 = { bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2,
                      bits32_2_2, bits32_2_2, bits32_2_2, bits32_2_2 };
  u32_512 incr2_3 = { bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3,
                      bits32_2_3, bits32_2_3, bits32_2_3, bits32_2_3 };
  u32_512 incr2_4 = { bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4,
                      bits32_2_4, bits32_2_4, bits32_2_4, bits32_2_4 };

  uint32_t bits32_3_1 = (uint32_t(bits3) & 0x0000FFFF);
  uint32_t bits32_3_2 = ((uint32_t(bits3) & 0xFFFF0000) >> 16);
  uint32_t bits32_3_3 = (uint64_t(bits3 >> 32) & 0x0000FFFF);
  uint32_t bits32_3_4 = ((uint64_t(bits3 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr3_1 = { bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1,
                      bits32_3_1, bits32_3_1, bits32_3_1, bits32_3_1 };
  u32_512 incr3_2 = { bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2,
                      bits32_3_2, bits32_3_2, bits32_3_2, bits32_3_2 };
  u32_512 incr3_3 = { bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3,
                      bits32_3_3, bits32_3_3, bits32_3_3, bits32_3_3 };
  u32_512 incr3_4 = { bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4,
                      bits32_3_4, bits32_3_4, bits32_3_4, bits32_3_4 };

  uint32_t bits32_4_1 = (uint32_t(bits4) & 0x0000FFFF);
  uint32_t bits32_4_2 = ((uint32_t(bits4) & 0xFFFF0000) >> 16);
  uint32_t bits32_4_3 = (uint64_t(bits4 >> 32) & 0x0000FFFF);
  uint32_t bits32_4_4 = ((uint64_t(bits4 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr4_1 = { bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1,
                      bits32_4_1, bits32_4_1, bits32_4_1, bits32_4_1 };
  u32_512 incr4_2 = { bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2,
                      bits32_4_2, bits32_4_2, bits32_4_2, bits32_4_2 };
  u32_512 incr4_3 = { bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3,
                      bits32_4_3, bits32_4_3, bits32_4_3, bits32_4_3 };
  u32_512 incr4_4 = { bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4,
                      bits32_4_4, bits32_4_4, bits32_4_4, bits32_4_4 };

  uint32_t bits32_5_1 = (uint32_t(bits5) & 0x0000FFFF);
  uint32_t bits32_5_2 = ((uint32_t(bits5) & 0xFFFF0000) >> 16);
  uint32_t bits32_5_3 = (uint64_t(bits5 >> 32) & 0x0000FFFF);
  uint32_t bits32_5_4 = ((uint64_t(bits5 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr5_1 = { bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1,
                      bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1,
                      bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1,
                      bits32_5_1, bits32_5_1, bits32_5_1, bits32_5_1 };
  u32_512 incr5_2 = { bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2,
                      bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2,
                      bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2,
                      bits32_5_2, bits32_5_2, bits32_5_2, bits32_5_2 };
  u32_512 incr5_3 = { bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3,
                      bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3,
                      bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3,
                      bits32_5_3, bits32_5_3, bits32_5_3, bits32_5_3 };
  u32_512 incr5_4 = { bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4,
                      bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4,
                      bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4,
                      bits32_5_4, bits32_5_4, bits32_5_4, bits32_5_4 };

  uint32_t bits32_6_1 = (uint32_t(bits6) & 0x0000FFFF);
  uint32_t bits32_6_2 = ((uint32_t(bits6) & 0xFFFF0000) >> 16);
  uint32_t bits32_6_3 = (uint64_t(bits6 >> 32) & 0x0000FFFF);
  uint32_t bits32_6_4 = ((uint64_t(bits6 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr6_1 = { bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1,
                      bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1,
                      bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1,
                      bits32_6_1, bits32_6_1, bits32_6_1, bits32_6_1 };
  u32_512 incr6_2 = { bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2,
                      bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2,
                      bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2,
                      bits32_6_2, bits32_6_2, bits32_6_2, bits32_6_2 };
  u32_512 incr6_3 = { bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3,
                      bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3,
                      bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3,
                      bits32_6_3, bits32_6_3, bits32_6_3, bits32_6_3 };
  u32_512 incr6_4 = { bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4,
                      bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4,
                      bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4,
                      bits32_6_4, bits32_6_4, bits32_6_4, bits32_6_4 };

  uint32_t bits32_7_1 = (uint32_t(bits7) & 0x0000FFFF);
  uint32_t bits32_7_2 = ((uint32_t(bits7) & 0xFFFF0000) >> 16);
  uint32_t bits32_7_3 = (uint64_t(bits7 >> 32) & 0x0000FFFF);
  uint32_t bits32_7_4 = ((uint64_t(bits7 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr7_1 = { bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1,
                      bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1,
                      bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1,
                      bits32_7_1, bits32_7_1, bits32_7_1, bits32_7_1 };
  u32_512 incr7_2 = { bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2,
                      bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2,
                      bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2,
                      bits32_7_2, bits32_7_2, bits32_7_2, bits32_7_2 };
  u32_512 incr7_3 = { bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3,
                      bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3,
                      bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3,
                      bits32_7_3, bits32_7_3, bits32_7_3, bits32_7_3 };
  u32_512 incr7_4 = { bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4,
                      bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4,
                      bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4,
                      bits32_7_4, bits32_7_4, bits32_7_4, bits32_7_4 };

  uint32_t bits32_8_1 = (uint32_t(bits8) & 0x0000FFFF);
  uint32_t bits32_8_2 = ((uint32_t(bits8) & 0xFFFF0000) >> 16);
  uint32_t bits32_8_3 = (uint64_t(bits8 >> 32) & 0x0000FFFF);
  uint32_t bits32_8_4 = ((uint64_t(bits8 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr8_1 = { bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1,
                      bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1,
                      bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1,
                      bits32_8_1, bits32_8_1, bits32_8_1, bits32_8_1 };
  u32_512 incr8_2 = { bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2,
                      bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2,
                      bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2,
                      bits32_8_2, bits32_8_2, bits32_8_2, bits32_8_2 };
  u32_512 incr8_3 = { bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3,
                      bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3,
                      bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3,
                      bits32_8_3, bits32_8_3, bits32_8_3, bits32_8_3 };
  u32_512 incr8_4 = { bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4,
                      bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4,
                      bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4,
                      bits32_8_4, bits32_8_4, bits32_8_4, bits32_8_4 };

  uint32_t bits32_9_1 = (uint32_t(bits9) & 0x0000FFFF);
  uint32_t bits32_9_2 = ((uint32_t(bits9) & 0xFFFF0000) >> 16);
  uint32_t bits32_9_3 = (uint64_t(bits9 >> 32) & 0x0000FFFF);
  uint32_t bits32_9_4 = ((uint64_t(bits9 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr9_1 = { bits32_9_1, bits32_9_1, bits32_9_1, bits32_9_1,
                      bits32_9_1, bits32_9_1, bits32_9_1, bits32_9_1,
                      bits32_9_1, bits32_9_1, bits32_9_1, bits32_9_1,
                      bits32_9_1, bits32_9_1, bits32_9_1, bits32_9_1 };
  u32_512 incr9_2 = { bits32_9_2, bits32_9_2, bits32_9_2, bits32_9_2,
                      bits32_9_2, bits32_9_2, bits32_9_2, bits32_9_2,
                      bits32_9_2, bits32_9_2, bits32_9_2, bits32_9_2,
                      bits32_9_2, bits32_9_2, bits32_9_2, bits32_9_2 };
  u32_512 incr9_3 = { bits32_9_3, bits32_9_3, bits32_9_3, bits32_9_3,
                      bits32_9_3, bits32_9_3, bits32_9_3, bits32_9_3,
                      bits32_9_3, bits32_9_3, bits32_9_3, bits32_9_3,
                      bits32_9_3, bits32_9_3, bits32_9_3, bits32_9_3 };
  u32_512 incr9_4 = { bits32_9_4, bits32_9_4, bits32_9_4, bits32_9_4,
                      bits32_9_4, bits32_9_4, bits32_9_4, bits32_9_4,
                      bits32_9_4, bits32_9_4, bits32_9_4, bits32_9_4,
                      bits32_9_4, bits32_9_4, bits32_9_4, bits32_9_4 };

  uint32_t bits32_10_1 = (uint32_t(bits10) & 0x0000FFFF);
  uint32_t bits32_10_2 = ((uint32_t(bits10) & 0xFFFF0000) >> 16);
  uint32_t bits32_10_3 = (uint64_t(bits10 >> 32) & 0x0000FFFF);
  uint32_t bits32_10_4 = ((uint64_t(bits10 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr10_1 = { bits32_10_1, bits32_10_1, bits32_10_1, bits32_10_1,
                       bits32_10_1, bits32_10_1, bits32_10_1, bits32_10_1,
                       bits32_10_1, bits32_10_1, bits32_10_1, bits32_10_1,
                       bits32_10_1, bits32_10_1, bits32_10_1, bits32_10_1 };
  u32_512 incr10_2 = { bits32_10_2, bits32_10_2, bits32_10_2, bits32_10_2,
                       bits32_10_2, bits32_10_2, bits32_10_2, bits32_10_2,
                       bits32_10_2, bits32_10_2, bits32_10_2, bits32_10_2,
                       bits32_10_2, bits32_10_2, bits32_10_2, bits32_10_2 };
  u32_512 incr10_3 = { bits32_10_3, bits32_10_3, bits32_10_3, bits32_10_3,
                       bits32_10_3, bits32_10_3, bits32_10_3, bits32_10_3,
                       bits32_10_3, bits32_10_3, bits32_10_3, bits32_10_3,
                       bits32_10_3, bits32_10_3, bits32_10_3, bits32_10_3 };
  u32_512 incr10_4 = { bits32_10_4, bits32_10_4, bits32_10_4, bits32_10_4,
                       bits32_10_4, bits32_10_4, bits32_10_4, bits32_10_4,
                       bits32_10_4, bits32_10_4, bits32_10_4, bits32_10_4,
                       bits32_10_4, bits32_10_4, bits32_10_4, bits32_10_4 };

  uint32_t bits32_11_1 = (uint32_t(bits11) & 0x0000FFFF);
  uint32_t bits32_11_2 = ((uint32_t(bits11) & 0xFFFF0000) >> 16);
  uint32_t bits32_11_3 = (uint64_t(bits11 >> 32) & 0x0000FFFF);
  uint32_t bits32_11_4 = ((uint64_t(bits11 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr11_1 = { bits32_11_1, bits32_11_1, bits32_11_1, bits32_11_1,
                       bits32_11_1, bits32_11_1, bits32_11_1, bits32_11_1,
                       bits32_11_1, bits32_11_1, bits32_11_1, bits32_11_1,
                       bits32_11_1, bits32_11_1, bits32_11_1, bits32_11_1 };
  u32_512 incr11_2 = { bits32_11_2, bits32_11_2, bits32_11_2, bits32_11_2,
                       bits32_11_2, bits32_11_2, bits32_11_2, bits32_11_2,
                       bits32_11_2, bits32_11_2, bits32_11_2, bits32_11_2,
                       bits32_11_2, bits32_11_2, bits32_11_2, bits32_11_2 };
  u32_512 incr11_3 = { bits32_11_3, bits32_11_3, bits32_11_3, bits32_11_3,
                       bits32_11_3, bits32_11_3, bits32_11_3, bits32_11_3,
                       bits32_11_3, bits32_11_3, bits32_11_3, bits32_11_3,
                       bits32_11_3, bits32_11_3, bits32_11_3, bits32_11_3 };
  u32_512 incr11_4 = { bits32_11_4, bits32_11_4, bits32_11_4, bits32_11_4,
                       bits32_11_4, bits32_11_4, bits32_11_4, bits32_11_4,
                       bits32_11_4, bits32_11_4, bits32_11_4, bits32_11_4,
                       bits32_11_4, bits32_11_4, bits32_11_4, bits32_11_4 };

  uint32_t bits32_12_1 = (uint32_t(bits12) & 0x0000FFFF);
  uint32_t bits32_12_2 = ((uint32_t(bits12) & 0xFFFF0000) >> 16);
  uint32_t bits32_12_3 = (uint64_t(bits12 >> 32) & 0x0000FFFF);
  uint32_t bits32_12_4 = ((uint64_t(bits12 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr12_1 = { bits32_12_1, bits32_12_1, bits32_12_1, bits32_12_1,
                       bits32_12_1, bits32_12_1, bits32_12_1, bits32_12_1,
                       bits32_12_1, bits32_12_1, bits32_12_1, bits32_12_1,
                       bits32_12_1, bits32_12_1, bits32_12_1, bits32_12_1 };
  u32_512 incr12_2 = { bits32_12_2, bits32_12_2, bits32_12_2, bits32_12_2,
                       bits32_12_2, bits32_12_2, bits32_12_2, bits32_12_2,
                       bits32_12_2, bits32_12_2, bits32_12_2, bits32_12_2,
                       bits32_12_2, bits32_12_2, bits32_12_2, bits32_12_2 };
  u32_512 incr12_3 = { bits32_12_3, bits32_12_3, bits32_12_3, bits32_12_3,
                       bits32_12_3, bits32_12_3, bits32_12_3, bits32_12_3,
                       bits32_12_3, bits32_12_3, bits32_12_3, bits32_12_3,
                       bits32_12_3, bits32_12_3, bits32_12_3, bits32_12_3 };
  u32_512 incr12_4 = { bits32_12_4, bits32_12_4, bits32_12_4, bits32_12_4,
                       bits32_12_4, bits32_12_4, bits32_12_4, bits32_12_4,
                       bits32_12_4, bits32_12_4, bits32_12_4, bits32_12_4,
                       bits32_12_4, bits32_12_4, bits32_12_4, bits32_12_4 };

  uint32_t bits32_13_1 = (uint32_t(bits13) & 0x0000FFFF);
  uint32_t bits32_13_2 = ((uint32_t(bits13) & 0xFFFF0000) >> 16);
  uint32_t bits32_13_3 = (uint64_t(bits13 >> 32) & 0x0000FFFF);
  uint32_t bits32_13_4 = ((uint64_t(bits13 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr13_1 = { bits32_13_1, bits32_13_1, bits32_13_1, bits32_13_1,
                       bits32_13_1, bits32_13_1, bits32_13_1, bits32_13_1,
                       bits32_13_1, bits32_13_1, bits32_13_1, bits32_13_1,
                       bits32_13_1, bits32_13_1, bits32_13_1, bits32_13_1 };
  u32_512 incr13_2 = { bits32_13_2, bits32_13_2, bits32_13_2, bits32_13_2,
                       bits32_13_2, bits32_13_2, bits32_13_2, bits32_13_2,
                       bits32_13_2, bits32_13_2, bits32_13_2, bits32_13_2,
                       bits32_13_2, bits32_13_2, bits32_13_2, bits32_13_2 };
  u32_512 incr13_3 = { bits32_13_3, bits32_13_3, bits32_13_3, bits32_13_3,
                       bits32_13_3, bits32_13_3, bits32_13_3, bits32_13_3,
                       bits32_13_3, bits32_13_3, bits32_13_3, bits32_13_3,
                       bits32_13_3, bits32_13_3, bits32_13_3, bits32_13_3 };
  u32_512 incr13_4 = { bits32_13_4, bits32_13_4, bits32_13_4, bits32_13_4,
                       bits32_13_4, bits32_13_4, bits32_13_4, bits32_13_4,
                       bits32_13_4, bits32_13_4, bits32_13_4, bits32_13_4,
                       bits32_13_4, bits32_13_4, bits32_13_4, bits32_13_4 };

  uint32_t bits32_14_1 = (uint32_t(bits14) & 0x0000FFFF);
  uint32_t bits32_14_2 = ((uint32_t(bits14) & 0xFFFF0000) >> 16);
  uint32_t bits32_14_3 = (uint64_t(bits14 >> 32) & 0x0000FFFF);
  uint32_t bits32_14_4 = ((uint64_t(bits14 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr14_1 = { bits32_14_1, bits32_14_1, bits32_14_1, bits32_14_1,
                       bits32_14_1, bits32_14_1, bits32_14_1, bits32_14_1,
                       bits32_14_1, bits32_14_1, bits32_14_1, bits32_14_1,
                       bits32_14_1, bits32_14_1, bits32_14_1, bits32_14_1 };
  u32_512 incr14_2 = { bits32_14_2, bits32_14_2, bits32_14_2, bits32_14_2,
                       bits32_14_2, bits32_14_2, bits32_14_2, bits32_14_2,
                       bits32_14_2, bits32_14_2, bits32_14_2, bits32_14_2,
                       bits32_14_2, bits32_14_2, bits32_14_2, bits32_14_2 };
  u32_512 incr14_3 = { bits32_14_3, bits32_14_3, bits32_14_3, bits32_14_3,
                       bits32_14_3, bits32_14_3, bits32_14_3, bits32_14_3,
                       bits32_14_3, bits32_14_3, bits32_14_3, bits32_14_3,
                       bits32_14_3, bits32_14_3, bits32_14_3, bits32_14_3 };
  u32_512 incr14_4 = { bits32_14_4, bits32_14_4, bits32_14_4, bits32_14_4,
                       bits32_14_4, bits32_14_4, bits32_14_4, bits32_14_4,
                       bits32_14_4, bits32_14_4, bits32_14_4, bits32_14_4,
                       bits32_14_4, bits32_14_4, bits32_14_4, bits32_14_4 };

  uint32_t bits32_15_1 = (uint32_t(bits15) & 0x0000FFFF);
  uint32_t bits32_15_2 = ((uint32_t(bits15) & 0xFFFF0000) >> 16);
  uint32_t bits32_15_3 = (uint64_t(bits15 >> 32) & 0x0000FFFF);
  uint32_t bits32_15_4 = ((uint64_t(bits15 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr15_1 = { bits32_15_1, bits32_15_1, bits32_15_1, bits32_15_1,
                       bits32_15_1, bits32_15_1, bits32_15_1, bits32_15_1,
                       bits32_15_1, bits32_15_1, bits32_15_1, bits32_15_1,
                       bits32_15_1, bits32_15_1, bits32_15_1, bits32_15_1 };
  u32_512 incr15_2 = { bits32_15_2, bits32_15_2, bits32_15_2, bits32_15_2,
                       bits32_15_2, bits32_15_2, bits32_15_2, bits32_15_2,
                       bits32_15_2, bits32_15_2, bits32_15_2, bits32_15_2,
                       bits32_15_2, bits32_15_2, bits32_15_2, bits32_15_2 };
  u32_512 incr15_3 = { bits32_15_3, bits32_15_3, bits32_15_3, bits32_15_3,
                       bits32_15_3, bits32_15_3, bits32_15_3, bits32_15_3,
                       bits32_15_3, bits32_15_3, bits32_15_3, bits32_15_3,
                       bits32_15_3, bits32_15_3, bits32_15_3, bits32_15_3 };
  u32_512 incr15_4 = { bits32_15_4, bits32_15_4, bits32_15_4, bits32_15_4,
                       bits32_15_4, bits32_15_4, bits32_15_4, bits32_15_4,
                       bits32_15_4, bits32_15_4, bits32_15_4, bits32_15_4,
                       bits32_15_4, bits32_15_4, bits32_15_4, bits32_15_4 };

  uint32_t bits32_16_1 = (uint32_t(bits16) & 0x0000FFFF);
  uint32_t bits32_16_2 = ((uint32_t(bits16) & 0xFFFF0000) >> 16);
  uint32_t bits32_16_3 = (uint64_t(bits16 >> 32) & 0x0000FFFF);
  uint32_t bits32_16_4 = ((uint64_t(bits16 >> 32) & 0xFFFF0000) >> 16);
  u32_512 incr16_1 = { bits32_16_1, bits32_16_1, bits32_16_1, bits32_16_1,
                       bits32_16_1, bits32_16_1, bits32_16_1, bits32_16_1,
                       bits32_16_1, bits32_16_1, bits32_16_1, bits32_16_1,
                       bits32_16_1, bits32_16_1, bits32_16_1, bits32_16_1 };
  u32_512 incr16_2 = { bits32_16_2, bits32_16_2, bits32_16_2, bits32_16_2,
                       bits32_16_2, bits32_16_2, bits32_16_2, bits32_16_2,
                       bits32_16_2, bits32_16_2, bits32_16_2, bits32_16_2,
                       bits32_16_2, bits32_16_2, bits32_16_2, bits32_16_2 };
  u32_512 incr16_3 = { bits32_16_3, bits32_16_3, bits32_16_3, bits32_16_3,
                       bits32_16_3, bits32_16_3, bits32_16_3, bits32_16_3,
                       bits32_16_3, bits32_16_3, bits32_16_3, bits32_16_3,
                       bits32_16_3, bits32_16_3, bits32_16_3, bits32_16_3 };
  u32_512 incr16_4 = { bits32_16_4, bits32_16_4, bits32_16_4, bits32_16_4,
                       bits32_16_4, bits32_16_4, bits32_16_4, bits32_16_4,
                       bits32_16_4, bits32_16_4, bits32_16_4, bits32_16_4,
                       bits32_16_4, bits32_16_4, bits32_16_4, bits32_16_4 };

  counts[blockIndex]     += ((incr1_1 & mask) >> shift) +
      ((incr2_1 & mask) >> shift) + ((incr3_1 & mask) >> shift) +
      ((incr4_1 & mask) >> shift) + ((incr5_1 & mask) >> shift) +
      ((incr6_1 & mask) >> shift) + ((incr7_1 & mask) >> shift) +
      ((incr8_1 & mask) >> shift) + ((incr9_1 & mask) >> shift) +
      ((incr10_1 & mask) >> shift) + ((incr11_1 & mask) >> shift) +
      ((incr12_1 & mask) >> shift) + ((incr13_1 & mask) >> shift) +
      ((incr14_1 & mask) >> shift) + ((incr15_1 & mask) >> shift) +
      ((incr16_1 & mask) >> shift);
  counts[blockIndex + 1] += ((incr1_2 & mask) >> shift) +
      ((incr2_2 & mask) >> shift) + ((incr3_2 & mask) >> shift) +
      ((incr4_2 & mask) >> shift) + ((incr5_2 & mask) >> shift) +
      ((incr6_2 & mask) >> shift) + ((incr7_2 & mask) >> shift) +
      ((incr8_2 & mask) >> shift) + ((incr9_2 & mask) >> shift) +
      ((incr10_2 & mask) >> shift) + ((incr11_2 & mask) >> shift) +
      ((incr12_2 & mask) >> shift) + ((incr13_2 & mask) >> shift) +
      ((incr14_2 & mask) >> shift) + ((incr15_2 & mask) >> shift) +
      ((incr16_2 & mask) >> shift);
  counts[blockIndex + 2] += ((incr1_3 & mask) >> shift) +
      ((incr2_3 & mask) >> shift) + ((incr3_3 & mask) >> shift) +
      ((incr4_3 & mask) >> shift) + ((incr5_3 & mask) >> shift) +
      ((incr6_3 & mask) >> shift) + ((incr7_3 & mask) >> shift) +
      ((incr8_3 & mask) >> shift) + ((incr9_3 & mask) >> shift) +
      ((incr10_3 & mask) >> shift) + ((incr11_3 & mask) >> shift) +
      ((incr12_3 & mask) >> shift) + ((incr13_3 & mask) >> shift) +
      ((incr14_3 & mask) >> shift) + ((incr15_3 & mask) >> shift) +
      ((incr16_3 & mask) >> shift);
  counts[blockIndex + 3] += ((incr1_4 & mask) >> shift) +
      ((incr2_4 & mask) >> shift) + ((incr3_4 & mask) >> shift) +
      ((incr4_4 & mask) >> shift) + ((incr5_4 & mask) >> shift) +
      ((incr6_4 & mask) >> shift) + ((incr7_4 & mask) >> shift) +
      ((incr8_4 & mask) >> shift) + ((incr9_4 & mask) >> shift) +
      ((incr10_4 & mask) >> shift) + ((incr11_4 & mask) >> shift) +
      ((incr12_4 & mask) >> shift) + ((incr13_4 & mask) >> shift) +
      ((incr14_4 & mask) >> shift) + ((incr15_4 & mask) >> shift) +
      ((incr16_4 & mask) >> shift);
}

template<bool FixedSize>
inline uint32_t CountsArray<FixedSize>::Maximum() const
{
  uint32_t maxVal = 0;
  for (size_t i = 0; i < 1048576; ++i)
    for (size_t j = 0; j < 16; ++j)
      maxVal = std::max(maxVal, (uint32_t) counts[i][j]);

  return maxVal;
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::Save(const std::string& filename) const
{
  std::fstream f(filename, std::fstream::out);
  uint32_t index = 0;
  for (size_t i = 0; i < 1048576; ++i)
  {
    for (size_t j = 0; j < 16; ++j, ++index)
    {
      f << "0x" << std::hex << std::setw(6) << std::setfill('0') << index
          << ", " << std::dec << (size_t) counts[i][j] << std::endl;
    }
  }
}

template<bool FixedSize>
inline uint32_t CountsArray<FixedSize>::operator[](const size_t i) const
{
  const size_t outerIndex = i / 16;
  const size_t innerIndex = i % 16;
  return counts[outerIndex][innerIndex];
}

template<bool FixedSize>
inline size_t CountsArray<FixedSize>::CopyPrefixes(
    uint8_t* prefixes,
    const uint32_t minValForCopy,
    size_t numWithMinValToCopy,
    uint32_t* prefixCounts) const
{
  size_t j = 0;
  if (prefixTrie != nullptr)
  {
    size_t i = 0;
    uint8_t* triePrefix = new uint8_t[prefixTrie->PrefixLen()];
    memset(triePrefix, 0x00, sizeof(uint8_t) * prefixTrie->PrefixLen());
    PrefixTrieIterate(prefixes, i, j, triePrefix, minValForCopy,
        numWithMinValToCopy, prefixCounts, 0, 0);
    delete[] triePrefix;
  }
  else
  {
    // Copy all relevant 3-gram prefixes.
    for (size_t i = 0; i < 16777216; ++i)
    {
      const size_t outerIndex = i / 16;
      const size_t innerIndex = i % 16;

      if (counts[outerIndex][innerIndex] > minValForCopy)
      {
        prefixes[3 * j    ] = (i & 0xFF0000) >> 16;
        prefixes[3 * j + 1] = (i & 0x00FF00) >> 8;
        prefixes[3 * j + 2] = (i & 0x0000FF);
        prefixCounts[j] = counts[outerIndex][innerIndex];

        ++j;
      }
      else if (counts[outerIndex][innerIndex] == minValForCopy &&
               numWithMinValToCopy != 0)
      {
        prefixes[3 * j    ] = (i & 0xFF0000) >> 16;
        prefixes[3 * j + 1] = (i & 0x00FF00) >> 8;
        prefixes[3 * j + 2] = (i & 0x0000FF);
        prefixCounts[j] = counts[outerIndex][innerIndex];

        --numWithMinValToCopy;
        ++j;
      }
    }
  }

  return j;
}

template<bool FixedSize>
inline void CountsArray<FixedSize>::PrefixTrieIterate(
    uint8_t* prefixes,
    size_t& leafIndex,
    size_t& j,
    uint8_t* triePrefix,
    const uint32_t minValForCopy,
    size_t& numWithMinValToCopy,
    uint32_t* prefixCounts,
    const size_t nodeIndex,
    const size_t depth) const
{
  // No need to set a prefix byte at the root.
  if (depth > 0)
  {
    triePrefix[depth - 1] = prefixTrie->bytes[nodeIndex];
    //std::cout << "iterating, leaf index " << leafIndex << ", depth " << depth << ", prefix trie bytes for node " << nodeIndex << " is "
    //    << std::hex << std::setw(2) << std::setfill('0') << (size_t) prefixTrie->bytes[nodeIndex]
    //    << "; trie prefix now 0x";
    //for (size_t i = 0; i < prefixTrie->PrefixLen(); ++i)
    //  std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) triePrefix[i];
    //std::cout << std::dec << "\n";
  }

  if (depth == prefixTrie->PrefixLen())
  {
    // This is a leaf.
    // Does it have a sufficiently large value for any of our 256 suffixes?
    bool any = false;
    for (size_t b = 0; b < 256; ++b)
    {
      const size_t prefixIndex = (leafIndex * 256) + b;

      const size_t outerIndex = prefixIndex / 16;
      const size_t innerIndex = prefixIndex % 16;

      if (counts[outerIndex][innerIndex] > 0)
        any = true;

      if (counts[outerIndex][innerIndex] > minValForCopy)
      {
        //std::cout << "counts leafIndex " << leafIndex << " prefixIndex " << prefixIndex << ", keep\n";
        memcpy(&prefixes[j * (prefixTrie->PrefixLen() + 1)], triePrefix,
            prefixTrie->PrefixLen());
        prefixes[j * (prefixTrie->PrefixLen() + 1) + prefixTrie->PrefixLen()] =
            b;
        prefixCounts[j] = counts[outerIndex][innerIndex];

        ++j;
      }
      else if (counts[outerIndex][innerIndex] == minValForCopy &&
               numWithMinValToCopy != 0)
      {
        //std::cout << "counts leafIndex " << leafIndex << " prefixIndex " << prefixIndex << ", numWithMinValToCopy " << numWithMinValToCopy << ", keep\n";
        memcpy(&prefixes[j * (prefixTrie->PrefixLen() + 1)], triePrefix,
            prefixTrie->PrefixLen());
        prefixes[j * (prefixTrie->PrefixLen() + 1) + prefixTrie->PrefixLen()] =
            b;
        prefixCounts[j] = counts[outerIndex][innerIndex];

        --numWithMinValToCopy;
        ++j;
      }
      //else
      //{
      //  std::cout << "prefix with byte " << b << " has count " << counts[outerIndex][innerIndex] << "\n";
      //}
    }

    if (!any)
      throw std::runtime_error("all counts for prefix are zero!");

    ++leafIndex;
  }
  else
  {
    const size_t numChildren = (prefixTrie->numChildren[nodeIndex] == prefixTrie->minChildrenForChildMap) ? 256 :
        prefixTrie->numChildren[nodeIndex];
    //std::cout << "node has " << numChildren << " children\n";

    // We need to iterate over the children in sorted order.
    std::pair<uint32_t, uint8_t> childPairs[256];
    memset(childPairs, 0x00, sizeof(std::pair<uint32_t, uint8_t>) * 256);
    for (size_t c = 0; c < numChildren; ++c)
    {
      if (prefixTrie->childrenVector[prefixTrie->children[nodeIndex] + c] == 0)
        childPairs[c] = std::make_pair(std::numeric_limits<uint32_t>::max(), c);
      else
        childPairs[c] = std::make_pair(prefixTrie->children[prefixTrie->childrenVector[prefixTrie->children[nodeIndex] + c]], c);
//      std::cout << "child pair " << c << ": " << (size_t) childPairs[c].first <<
//", " << (size_t) childPairs[c].second << "\n";
    }
    std::stable_sort(childPairs, childPairs + numChildren);

    for (size_t c = 0; c < numChildren; ++c)
    {
      const size_t cInd = childPairs[c].second;
      const size_t childNodeIndex =
          prefixTrie->childrenVector[prefixTrie->children[nodeIndex] + cInd];
      //std::cout << "child " << std::dec << c << " (" << cInd << ") at depth " << depth << " has index " << childNodeIndex << "\n";

      if (childNodeIndex != 0)
        PrefixTrieIterate(prefixes, leafIndex, j, triePrefix, minValForCopy,
            numWithMinValToCopy, prefixCounts, childNodeIndex, depth + 1);
    }
  }
}

#endif
