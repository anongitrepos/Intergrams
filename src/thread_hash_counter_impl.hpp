#ifndef THREAD_HASH_COUNTER_IMPL_HPP
#define THREAD_HASH_COUNTER_IMPL_HPP

#include "thread_hash_counter.hpp"
#include <cstring>
#include <iostream>

inline ThreadHashCounter::ThreadHashCounter()
{
  clear();
}

inline void ThreadHashCounter::set(const unsigned char* b)
{
  const size_t index = ((size_t(*b) << 16) + (size_t(*(b + 1)) << 8) +
      size_t(*(b + 2)));

  const size_t bitsIndex = index / 64;
  const size_t bit = index & 0x3F;
  bits[bitsIndex] |= (uint64_t(1) << bit);
}

inline void ThreadHashCounter::clear()
{
  memset(bits, 0, sizeof(uint64_t) * 262144);
}

inline void ThreadHashCounter::flush(std::atomic_uint32_t* gramCounts)
{
  for (size_t i = 0; i < 262144; ++i)
  {
    const uint64_t& b = bits[i];
    if (b == 0)
      continue;

    if ((b & 0x0000000000000001) == 0x0000000000000001)
      ++gramCounts[64 * i];
    if ((b & 0x0000000000000002) == 0x0000000000000002)
      ++gramCounts[64 * i + 1];
    if ((b & 0x0000000000000004) == 0x0000000000000004)
      ++gramCounts[64 * i + 2];
    if ((b & 0x0000000000000008) == 0x0000000000000008)
      ++gramCounts[64 * i + 3];
    if ((b & 0x0000000000000010) == 0x0000000000000010)
      ++gramCounts[64 * i + 4];
    if ((b & 0x0000000000000020) == 0x0000000000000020)
      ++gramCounts[64 * i + 5];
    if ((b & 0x0000000000000040) == 0x0000000000000040)
      ++gramCounts[64 * i + 6];
    if ((b & 0x0000000000000080) == 0x0000000000000080)
      ++gramCounts[64 * i + 7];
    if ((b & 0x0000000000000100) == 0x0000000000000100)
      ++gramCounts[64 * i + 8];
    if ((b & 0x0000000000000200) == 0x0000000000000200)
      ++gramCounts[64 * i + 9];
    if ((b & 0x0000000000000400) == 0x0000000000000400)
      ++gramCounts[64 * i + 10];
    if ((b & 0x0000000000000800) == 0x0000000000000800)
      ++gramCounts[64 * i + 11];
    if ((b & 0x0000000000001000) == 0x0000000000001000)
      ++gramCounts[64 * i + 12];
    if ((b & 0x0000000000002000) == 0x0000000000002000)
      ++gramCounts[64 * i + 13];
    if ((b & 0x0000000000004000) == 0x0000000000004000)
      ++gramCounts[64 * i + 14];
    if ((b & 0x0000000000008000) == 0x0000000000008000)
      ++gramCounts[64 * i + 15];
    if ((b & 0x0000000000010000) == 0x0000000000010000)
      ++gramCounts[64 * i + 16];
    if ((b & 0x0000000000020000) == 0x0000000000020000)
      ++gramCounts[64 * i + 17];
    if ((b & 0x0000000000040000) == 0x0000000000040000)
      ++gramCounts[64 * i + 18];
    if ((b & 0x0000000000080000) == 0x0000000000080000)
      ++gramCounts[64 * i + 19];
    if ((b & 0x0000000000100000) == 0x0000000000100000)
      ++gramCounts[64 * i + 20];
    if ((b & 0x0000000000200000) == 0x0000000000200000)
      ++gramCounts[64 * i + 21];
    if ((b & 0x0000000000400000) == 0x0000000000400000)
      ++gramCounts[64 * i + 22];
    if ((b & 0x0000000000800000) == 0x0000000000800000)
      ++gramCounts[64 * i + 23];
    if ((b & 0x0000000001000000) == 0x0000000001000000)
      ++gramCounts[64 * i + 24];
    if ((b & 0x0000000002000000) == 0x0000000002000000)
      ++gramCounts[64 * i + 25];
    if ((b & 0x0000000004000000) == 0x0000000004000000)
      ++gramCounts[64 * i + 26];
    if ((b & 0x0000000008000000) == 0x0000000008000000)
      ++gramCounts[64 * i + 27];
    if ((b & 0x0000000010000000) == 0x0000000010000000)
      ++gramCounts[64 * i + 28];
    if ((b & 0x0000000020000000) == 0x0000000020000000)
      ++gramCounts[64 * i + 29];
    if ((b & 0x0000000040000000) == 0x0000000040000000)
      ++gramCounts[64 * i + 30];
    if ((b & 0x0000000080000000) == 0x0000000080000000)
      ++gramCounts[64 * i + 31];
    if ((b & 0x0000000100000000) == 0x0000000100000000)
      ++gramCounts[64 * i + 32];
    if ((b & 0x0000000200000000) == 0x0000000200000000)
      ++gramCounts[64 * i + 33];
    if ((b & 0x0000000400000000) == 0x0000000400000000)
      ++gramCounts[64 * i + 34];
    if ((b & 0x0000000800000000) == 0x0000000800000000)
      ++gramCounts[64 * i + 35];
    if ((b & 0x0000001000000000) == 0x0000001000000000)
      ++gramCounts[64 * i + 36];
    if ((b & 0x0000002000000000) == 0x0000002000000000)
      ++gramCounts[64 * i + 37];
    if ((b & 0x0000004000000000) == 0x0000004000000000)
      ++gramCounts[64 * i + 38];
    if ((b & 0x0000008000000000) == 0x0000008000000000)
      ++gramCounts[64 * i + 39];
    if ((b & 0x0000010000000000) == 0x0000010000000000)
      ++gramCounts[64 * i + 40];
    if ((b & 0x0000020000000000) == 0x0000020000000000)
      ++gramCounts[64 * i + 41];
    if ((b & 0x0000040000000000) == 0x0000040000000000)
      ++gramCounts[64 * i + 42];
    if ((b & 0x0000080000000000) == 0x0000080000000000)
      ++gramCounts[64 * i + 43];
    if ((b & 0x0000100000000000) == 0x0000100000000000)
      ++gramCounts[64 * i + 44];
    if ((b & 0x0000200000000000) == 0x0000200000000000)
      ++gramCounts[64 * i + 45];
    if ((b & 0x0000400000000000) == 0x0000400000000000)
      ++gramCounts[64 * i + 46];
    if ((b & 0x0000800000000000) == 0x0000800000000000)
      ++gramCounts[64 * i + 47];
    if ((b & 0x0001000000000000) == 0x0001000000000000)
      ++gramCounts[64 * i + 48];
    if ((b & 0x0002000000000000) == 0x0002000000000000)
      ++gramCounts[64 * i + 49];
    if ((b & 0x0004000000000000) == 0x0004000000000000)
      ++gramCounts[64 * i + 50];
    if ((b & 0x0008000000000000) == 0x0008000000000000)
      ++gramCounts[64 * i + 51];
    if ((b & 0x0010000000000000) == 0x0010000000000000)
      ++gramCounts[64 * i + 52];
    if ((b & 0x0020000000000000) == 0x0020000000000000)
      ++gramCounts[64 * i + 53];
    if ((b & 0x0040000000000000) == 0x0040000000000000)
      ++gramCounts[64 * i + 54];
    if ((b & 0x0080000000000000) == 0x0080000000000000)
      ++gramCounts[64 * i + 55];
    if ((b & 0x0100000000000000) == 0x0100000000000000)
      ++gramCounts[64 * i + 56];
    if ((b & 0x0200000000000000) == 0x0200000000000000)
      ++gramCounts[64 * i + 57];
    if ((b & 0x0400000000000000) == 0x0400000000000000)
      ++gramCounts[64 * i + 58];
    if ((b & 0x0800000000000000) == 0x0800000000000000)
      ++gramCounts[64 * i + 59];
    if ((b & 0x1000000000000000) == 0x1000000000000000)
      ++gramCounts[64 * i + 60];
    if ((b & 0x2000000000000000) == 0x2000000000000000)
      ++gramCounts[64 * i + 61];
    if ((b & 0x4000000000000000) == 0x4000000000000000)
      ++gramCounts[64 * i + 62];
    if ((b & 0x8000000000000000) == 0x8000000000000000)
      ++gramCounts[64 * i + 63];
  }

  clear();
}

inline void ThreadHashCounter::flush(CountsArray<>& countsArray)
{
  for (size_t i = 0; i < 262144; ++i)
  {
    if (bits[i] != 0)
      countsArray.Increment(i, bits[i]);
  }

  clear();
}

#endif
