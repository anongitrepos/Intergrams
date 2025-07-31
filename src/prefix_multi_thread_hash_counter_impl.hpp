#ifndef PNGRAM_PREFIX_MULTI_THREAD_HASH_COUNTER_IMPL_HPP
#define PNGRAM_PREFIX_MULTI_THREAD_HASH_COUNTER_IMPL_HPP

#include "prefix_multi_thread_hash_counter.hpp"
#include <cstring>
#include <iostream>

inline PrefixMultiThreadHashCounter::PrefixMultiThreadHashCounter(
    const size_t elem,
    const PackedByteTrie<uint32_t>* prefixTrieIn,
    const size_t prefixLenIn) :
    bitsIndex(0),
    prefixTrie(prefixTrieIn),
    prefixLen(prefixLenIn),
    bitsetLen((elem + 63) / 64)
{
  // Allocate bitsets.  8 bitsets, one bit per element.
  alloc_hugepage<uint64_t>(bits, bitsMemState, 8 * bitsetLen, "n-gramming");

  clear();
}

inline PrefixMultiThreadHashCounter::~PrefixMultiThreadHashCounter()
{
  free_hugepage<uint64_t>(bits, bitsMemState, 8 * bitsetLen);
}

inline void PrefixMultiThreadHashCounter::set(const unsigned char* b)
{
  const size_t prefixId = prefixTrie->Search(b);
  if (prefixId == size_t(-1))
    return; // not a prefix we care about
  //std::cout << "byte sequence 0x";
  //for (size_t i = 0; i < prefixTrie->PrefixLen(); ++i)
  //  std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) b[i];
  //std::cout << std::dec << " has prefix ID " << prefixId << "\n";

  // Get the index of the n-gram.  Each prefix is associated with 256 possible
  // n-grams, so use the index stored in the prefix plus the last byte to get
  // the actual index in `bits`.
  const size_t index = 256 * prefixId + b[prefixLen];

  const size_t bitLoc = index / 64;
  const size_t bit = index & 0x3F;
  bits[bitsIndex * bitsetLen + bitLoc] |= (uint64_t(1) << bit);
}

inline void PrefixMultiThreadHashCounter::clear()
{
  memset(bits, 0, sizeof(uint64_t) * 8 * bitsetLen);
}

template<bool FixedSize>
inline void PrefixMultiThreadHashCounter::flush(CountsArray<FixedSize>& countsArray)
{
  ++bitsIndex;
  if (bitsIndex == 8)
    bitsIndex = 0;

  if (bitsIndex != 0)
    return;

  for (size_t i = 0; i < bitsetLen; ++i)
  {
    if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0 ||
        bits[3 * bitsetLen + i] != 0 || bits[4 * bitsetLen + i] != 0 || bits[5 * bitsetLen + i] != 0 ||
        bits[6 * bitsetLen + i] != 0 || bits[7 * bitsetLen + i] != 0)
    {
      countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i],
          bits[2 * bitsetLen + i], bits[3 * bitsetLen + i], bits[4 * bitsetLen + i],
          bits[5 * bitsetLen + i], bits[6 * bitsetLen + i], bits[7 * bitsetLen + i]);

      bits[0 * bitsetLen + i] = 0;
      bits[1 * bitsetLen + i] = 0;
      bits[2 * bitsetLen + i] = 0;
      bits[3 * bitsetLen + i] = 0;
      bits[4 * bitsetLen + i] = 0;
      bits[5 * bitsetLen + i] = 0;
      bits[6 * bitsetLen + i] = 0;
      bits[7 * bitsetLen + i] = 0;
    }
  }
}

template<bool FixedSize>
inline void PrefixMultiThreadHashCounter::forceFlush(CountsArray<FixedSize>& countsArray)
{
  if (bitsIndex == 1)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[i] != 0)
        countsArray.Increment(i, bits[i]);
    }
  }
  else if (bitsIndex == 2)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0)
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i]);
    }
  }
  else if (bitsIndex == 3)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0)
      {
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i], bits[2 * bitsetLen + i],
            (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 4)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0 ||
          bits[3 * bitsetLen + i] != 0)
      {
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i],
            bits[2 * bitsetLen + i], bits[3 * bitsetLen + i]);
      }
    }
  }
  else if (bitsIndex == 5)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0 ||
          bits[3 * bitsetLen + i] != 0 || bits[4 * bitsetLen + i] != 0)
      {
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i], bits[2 * bitsetLen + i],
            bits[3 * bitsetLen + i], bits[4 * bitsetLen + i], (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 6)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0 ||
          bits[3 * bitsetLen + i] != 0 || bits[4 * bitsetLen + i] != 0 || bits[5 * bitsetLen + i] != 0)
      {
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i], bits[2 * bitsetLen + i],
            bits[3 * bitsetLen + i], bits[4 * bitsetLen + i],
            bits[5 * bitsetLen + i], (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 7)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0 ||
          bits[3 * bitsetLen + i] != 0 || bits[4 * bitsetLen + i] != 0 || bits[5 * bitsetLen + i] != 0 ||
          bits[6 * bitsetLen + i] != 0)
      {
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i], bits[2 * bitsetLen + i],
            bits[3 * bitsetLen + i], bits[4 * bitsetLen + i], bits[5 * bitsetLen + i], bits[6 * bitsetLen + i], (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 8)
  {
    for (size_t i = 0; i < bitsetLen; ++i)
    {
      if (bits[0 * bitsetLen + i] != 0 || bits[1 * bitsetLen + i] != 0 || bits[2 * bitsetLen + i] != 0 ||
          bits[3 * bitsetLen + i] != 0 || bits[4 * bitsetLen + i] != 0 || bits[5 * bitsetLen + i] != 0 ||
          bits[6 * bitsetLen + i] != 0 || bits[7 * bitsetLen + i] != 0)
      {
        countsArray.Increment(i, bits[0 * bitsetLen + i], bits[1 * bitsetLen + i], bits[2 * bitsetLen + i],
            bits[3 * bitsetLen + i], bits[4 * bitsetLen + i], bits[5 * bitsetLen + i], bits[6 * bitsetLen + i], bits[7 * bitsetLen + i]);
      }
    }
  }
  // not implemented for greater than 8...
}

#endif
