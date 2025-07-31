#ifndef PNGRAM_MULTI_THREAD_HASH_COUNTER_IMPL_HPP
#define PNGRAM_MULTI_THREAD_HASH_COUNTER_IMPL_HPP

#include "multi_thread_hash_counter.hpp"
#include <cstring>
#include <iostream>

inline MultiThreadHashCounter::MultiThreadHashCounter() :
    bitsIndex(0)
{
  clear();
}

inline void MultiThreadHashCounter::set(const unsigned char* b)
{
  const size_t index = ((size_t(*b) << 16) + (size_t(*(b + 1)) << 8) +
      size_t(*(b + 2)));

  const size_t bitLoc = index / 64;
  const size_t bit = index & 0x3F;
  bits[bitsIndex][bitLoc] |= (uint64_t(1) << bit);
}

inline void MultiThreadHashCounter::clear()
{
  memset(bits, 0, sizeof(uint64_t) * 8 * 262144);
}

inline void MultiThreadHashCounter::flush(std::atomic_uint32_t* gramCounts)
{
  ++bitsIndex;
  if (bitsIndex == 2)
    bitsIndex = 0;

  if (bitsIndex == 1)
    return;

  for (size_t i = 0; i < 262144; ++i)
  {
    __builtin_prefetch(&gramCounts[64 * (i + 1)], 1, 3);

    const uint64_t& b1 = bits[0][i];
    const uint64_t& b2 = bits[1][i];
    if (b1 == 0 && b2 == 0)
      continue;

    if ((b1 & 0x0000000000000001) == 0x0000000000000001)
      ++gramCounts[64 * i];
    if ((b2 & 0x0000000000000001) == 0x0000000000000001)
      ++gramCounts[64 * i];
    if ((b1 & 0x0000000000000002) == 0x0000000000000002)
      ++gramCounts[64 * i + 1];
    if ((b2 & 0x0000000000000002) == 0x0000000000000002)
      ++gramCounts[64 * i + 1];
    if ((b1 & 0x0000000000000004) == 0x0000000000000004)
      ++gramCounts[64 * i + 2];
    if ((b2 & 0x0000000000000004) == 0x0000000000000004)
      ++gramCounts[64 * i + 2];
    if ((b1 & 0x0000000000000008) == 0x0000000000000008)
      ++gramCounts[64 * i + 3];
    if ((b2 & 0x0000000000000008) == 0x0000000000000008)
      ++gramCounts[64 * i + 3];
    if ((b1 & 0x0000000000000010) == 0x0000000000000010)
      ++gramCounts[64 * i + 4];
    if ((b2 & 0x0000000000000010) == 0x0000000000000010)
      ++gramCounts[64 * i + 4];
    if ((b1 & 0x0000000000000020) == 0x0000000000000020)
      ++gramCounts[64 * i + 5];
    if ((b2 & 0x0000000000000020) == 0x0000000000000020)
      ++gramCounts[64 * i + 5];
    if ((b1 & 0x0000000000000040) == 0x0000000000000040)
      ++gramCounts[64 * i + 6];
    if ((b2 & 0x0000000000000040) == 0x0000000000000040)
      ++gramCounts[64 * i + 6];
    if ((b1 & 0x0000000000000080) == 0x0000000000000080)
      ++gramCounts[64 * i + 7];
    if ((b2 & 0x0000000000000080) == 0x0000000000000080)
      ++gramCounts[64 * i + 7];
    if ((b1 & 0x0000000000000100) == 0x0000000000000100)
      ++gramCounts[64 * i + 8];
    if ((b2 & 0x0000000000000100) == 0x0000000000000100)
      ++gramCounts[64 * i + 8];
    if ((b1 & 0x0000000000000200) == 0x0000000000000200)
      ++gramCounts[64 * i + 9];
    if ((b2 & 0x0000000000000200) == 0x0000000000000200)
      ++gramCounts[64 * i + 9];
    if ((b1 & 0x0000000000000400) == 0x0000000000000400)
      ++gramCounts[64 * i + 10];
    if ((b2 & 0x0000000000000400) == 0x0000000000000400)
      ++gramCounts[64 * i + 10];
    if ((b1 & 0x0000000000000800) == 0x0000000000000800)
      ++gramCounts[64 * i + 11];
    if ((b2 & 0x0000000000000800) == 0x0000000000000800)
      ++gramCounts[64 * i + 11];
    if ((b1 & 0x0000000000001000) == 0x0000000000001000)
      ++gramCounts[64 * i + 12];
    if ((b2 & 0x0000000000001000) == 0x0000000000001000)
      ++gramCounts[64 * i + 12];
    if ((b1 & 0x0000000000002000) == 0x0000000000002000)
      ++gramCounts[64 * i + 13];
    if ((b2 & 0x0000000000002000) == 0x0000000000002000)
      ++gramCounts[64 * i + 13];
    if ((b1 & 0x0000000000004000) == 0x0000000000004000)
      ++gramCounts[64 * i + 14];
    if ((b2 & 0x0000000000004000) == 0x0000000000004000)
      ++gramCounts[64 * i + 14];
    if ((b1 & 0x0000000000008000) == 0x0000000000008000)
      ++gramCounts[64 * i + 15];
    if ((b2 & 0x0000000000008000) == 0x0000000000008000)
      ++gramCounts[64 * i + 15];
    if ((b1 & 0x0000000000010000) == 0x0000000000010000)
      ++gramCounts[64 * i + 16];
    if ((b2 & 0x0000000000010000) == 0x0000000000010000)
      ++gramCounts[64 * i + 16];
    if ((b1 & 0x0000000000020000) == 0x0000000000020000)
      ++gramCounts[64 * i + 17];
    if ((b2 & 0x0000000000020000) == 0x0000000000020000)
      ++gramCounts[64 * i + 17];
    if ((b1 & 0x0000000000040000) == 0x0000000000040000)
      ++gramCounts[64 * i + 18];
    if ((b2 & 0x0000000000040000) == 0x0000000000040000)
      ++gramCounts[64 * i + 18];
    if ((b1 & 0x0000000000080000) == 0x0000000000080000)
      ++gramCounts[64 * i + 19];
    if ((b2 & 0x0000000000080000) == 0x0000000000080000)
      ++gramCounts[64 * i + 19];
    if ((b1 & 0x0000000000100000) == 0x0000000000100000)
      ++gramCounts[64 * i + 20];
    if ((b2 & 0x0000000000100000) == 0x0000000000100000)
      ++gramCounts[64 * i + 20];
    if ((b1 & 0x0000000000200000) == 0x0000000000200000)
      ++gramCounts[64 * i + 21];
    if ((b2 & 0x0000000000200000) == 0x0000000000200000)
      ++gramCounts[64 * i + 21];
    if ((b1 & 0x0000000000400000) == 0x0000000000400000)
      ++gramCounts[64 * i + 22];
    if ((b2 & 0x0000000000400000) == 0x0000000000400000)
      ++gramCounts[64 * i + 22];
    if ((b1 & 0x0000000000800000) == 0x0000000000800000)
      ++gramCounts[64 * i + 23];
    if ((b2 & 0x0000000000800000) == 0x0000000000800000)
      ++gramCounts[64 * i + 23];
    if ((b1 & 0x0000000001000000) == 0x0000000001000000)
      ++gramCounts[64 * i + 24];
    if ((b2 & 0x0000000001000000) == 0x0000000001000000)
      ++gramCounts[64 * i + 24];
    if ((b1 & 0x0000000002000000) == 0x0000000002000000)
      ++gramCounts[64 * i + 25];
    if ((b2 & 0x0000000002000000) == 0x0000000002000000)
      ++gramCounts[64 * i + 25];
    if ((b1 & 0x0000000004000000) == 0x0000000004000000)
      ++gramCounts[64 * i + 26];
    if ((b2 & 0x0000000004000000) == 0x0000000004000000)
      ++gramCounts[64 * i + 26];
    if ((b1 & 0x0000000008000000) == 0x0000000008000000)
      ++gramCounts[64 * i + 27];
    if ((b2 & 0x0000000008000000) == 0x0000000008000000)
      ++gramCounts[64 * i + 27];
    if ((b1 & 0x0000000010000000) == 0x0000000010000000)
      ++gramCounts[64 * i + 28];
    if ((b2 & 0x0000000010000000) == 0x0000000010000000)
      ++gramCounts[64 * i + 28];
    if ((b1 & 0x0000000020000000) == 0x0000000020000000)
      ++gramCounts[64 * i + 29];
    if ((b2 & 0x0000000020000000) == 0x0000000020000000)
      ++gramCounts[64 * i + 29];
    if ((b1 & 0x0000000040000000) == 0x0000000040000000)
      ++gramCounts[64 * i + 30];
    if ((b2 & 0x0000000040000000) == 0x0000000040000000)
      ++gramCounts[64 * i + 30];
    if ((b1 & 0x0000000080000000) == 0x0000000080000000)
      ++gramCounts[64 * i + 31];
    if ((b2 & 0x0000000080000000) == 0x0000000080000000)
      ++gramCounts[64 * i + 31];
    if ((b1 & 0x0000000100000000) == 0x0000000100000000)
      ++gramCounts[64 * i + 32];
    if ((b2 & 0x0000000100000000) == 0x0000000100000000)
      ++gramCounts[64 * i + 32];
    if ((b1 & 0x0000000200000000) == 0x0000000200000000)
      ++gramCounts[64 * i + 33];
    if ((b2 & 0x0000000200000000) == 0x0000000200000000)
      ++gramCounts[64 * i + 33];
    if ((b1 & 0x0000000400000000) == 0x0000000400000000)
      ++gramCounts[64 * i + 34];
    if ((b2 & 0x0000000400000000) == 0x0000000400000000)
      ++gramCounts[64 * i + 34];
    if ((b1 & 0x0000000800000000) == 0x0000000800000000)
      ++gramCounts[64 * i + 35];
    if ((b2 & 0x0000000800000000) == 0x0000000800000000)
      ++gramCounts[64 * i + 35];
    if ((b1 & 0x0000001000000000) == 0x0000001000000000)
      ++gramCounts[64 * i + 36];
    if ((b2 & 0x0000001000000000) == 0x0000001000000000)
      ++gramCounts[64 * i + 36];
    if ((b1 & 0x0000002000000000) == 0x0000002000000000)
      ++gramCounts[64 * i + 37];
    if ((b2 & 0x0000002000000000) == 0x0000002000000000)
      ++gramCounts[64 * i + 37];
    if ((b1 & 0x0000004000000000) == 0x0000004000000000)
      ++gramCounts[64 * i + 38];
    if ((b2 & 0x0000004000000000) == 0x0000004000000000)
      ++gramCounts[64 * i + 38];
    if ((b1 & 0x0000008000000000) == 0x0000008000000000)
      ++gramCounts[64 * i + 39];
    if ((b2 & 0x0000008000000000) == 0x0000008000000000)
      ++gramCounts[64 * i + 39];
    if ((b1 & 0x0000010000000000) == 0x0000010000000000)
      ++gramCounts[64 * i + 40];
    if ((b2 & 0x0000010000000000) == 0x0000010000000000)
      ++gramCounts[64 * i + 40];
    if ((b1 & 0x0000020000000000) == 0x0000020000000000)
      ++gramCounts[64 * i + 41];
    if ((b2 & 0x0000020000000000) == 0x0000020000000000)
      ++gramCounts[64 * i + 41];
    if ((b1 & 0x0000040000000000) == 0x0000040000000000)
      ++gramCounts[64 * i + 42];
    if ((b2 & 0x0000040000000000) == 0x0000040000000000)
      ++gramCounts[64 * i + 42];
    if ((b1 & 0x0000080000000000) == 0x0000080000000000)
      ++gramCounts[64 * i + 43];
    if ((b2 & 0x0000080000000000) == 0x0000080000000000)
      ++gramCounts[64 * i + 43];
    if ((b1 & 0x0000100000000000) == 0x0000100000000000)
      ++gramCounts[64 * i + 44];
    if ((b2 & 0x0000100000000000) == 0x0000100000000000)
      ++gramCounts[64 * i + 44];
    if ((b1 & 0x0000200000000000) == 0x0000200000000000)
      ++gramCounts[64 * i + 45];
    if ((b2 & 0x0000200000000000) == 0x0000200000000000)
      ++gramCounts[64 * i + 45];
    if ((b1 & 0x0000400000000000) == 0x0000400000000000)
      ++gramCounts[64 * i + 46];
    if ((b2 & 0x0000400000000000) == 0x0000400000000000)
      ++gramCounts[64 * i + 46];
    if ((b1 & 0x0000800000000000) == 0x0000800000000000)
      ++gramCounts[64 * i + 47];
    if ((b2 & 0x0000800000000000) == 0x0000800000000000)
      ++gramCounts[64 * i + 47];
    if ((b1 & 0x0001000000000000) == 0x0001000000000000)
      ++gramCounts[64 * i + 48];
    if ((b2 & 0x0001000000000000) == 0x0001000000000000)
      ++gramCounts[64 * i + 48];
    if ((b1 & 0x0002000000000000) == 0x0002000000000000)
      ++gramCounts[64 * i + 49];
    if ((b2 & 0x0002000000000000) == 0x0002000000000000)
      ++gramCounts[64 * i + 49];
    if ((b1 & 0x0004000000000000) == 0x0004000000000000)
      ++gramCounts[64 * i + 50];
    if ((b2 & 0x0004000000000000) == 0x0004000000000000)
      ++gramCounts[64 * i + 50];
    if ((b1 & 0x0008000000000000) == 0x0008000000000000)
      ++gramCounts[64 * i + 51];
    if ((b2 & 0x0008000000000000) == 0x0008000000000000)
      ++gramCounts[64 * i + 51];
    if ((b1 & 0x0010000000000000) == 0x0010000000000000)
      ++gramCounts[64 * i + 52];
    if ((b2 & 0x0010000000000000) == 0x0010000000000000)
      ++gramCounts[64 * i + 52];
    if ((b1 & 0x0020000000000000) == 0x0020000000000000)
      ++gramCounts[64 * i + 53];
    if ((b2 & 0x0020000000000000) == 0x0020000000000000)
      ++gramCounts[64 * i + 53];
    if ((b1 & 0x0040000000000000) == 0x0040000000000000)
      ++gramCounts[64 * i + 54];
    if ((b2 & 0x0040000000000000) == 0x0040000000000000)
      ++gramCounts[64 * i + 54];
    if ((b1 & 0x0080000000000000) == 0x0080000000000000)
      ++gramCounts[64 * i + 55];
    if ((b2 & 0x0080000000000000) == 0x0080000000000000)
      ++gramCounts[64 * i + 55];
    if ((b1 & 0x0100000000000000) == 0x0100000000000000)
      ++gramCounts[64 * i + 56];
    if ((b2 & 0x0100000000000000) == 0x0100000000000000)
      ++gramCounts[64 * i + 56];
    if ((b1 & 0x0200000000000000) == 0x0200000000000000)
      ++gramCounts[64 * i + 57];
    if ((b2 & 0x0200000000000000) == 0x0200000000000000)
      ++gramCounts[64 * i + 57];
    if ((b1 & 0x0400000000000000) == 0x0400000000000000)
      ++gramCounts[64 * i + 58];
    if ((b2 & 0x0400000000000000) == 0x0400000000000000)
      ++gramCounts[64 * i + 58];
    if ((b1 & 0x0800000000000000) == 0x0800000000000000)
      ++gramCounts[64 * i + 59];
    if ((b2 & 0x0800000000000000) == 0x0800000000000000)
      ++gramCounts[64 * i + 59];
    if ((b1 & 0x1000000000000000) == 0x1000000000000000)
      ++gramCounts[64 * i + 60];
    if ((b2 & 0x1000000000000000) == 0x1000000000000000)
      ++gramCounts[64 * i + 60];
    if ((b1 & 0x2000000000000000) == 0x2000000000000000)
      ++gramCounts[64 * i + 61];
    if ((b2 & 0x2000000000000000) == 0x2000000000000000)
      ++gramCounts[64 * i + 61];
    if ((b1 & 0x4000000000000000) == 0x4000000000000000)
      ++gramCounts[64 * i + 62];
    if ((b2 & 0x4000000000000000) == 0x4000000000000000)
      ++gramCounts[64 * i + 62];
    if ((b1 & 0x8000000000000000) == 0x8000000000000000)
      ++gramCounts[64 * i + 63];
    if ((b2 & 0x8000000000000000) == 0x8000000000000000)
      ++gramCounts[64 * i + 63];
  }

  clear();
}

inline void MultiThreadHashCounter::flush(CountsArray<>& countsArray)
{
  ++bitsIndex;
  if (bitsIndex == 8)
    bitsIndex = 0;

  if (bitsIndex != 0)
    return;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
        bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
        bits[6][i] != 0 || bits[7][i] != 0)
    {
      countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i], bits[3][i],
          bits[4][i], bits[5][i], bits[6][i], bits[7][i]);

      bits[0][i] = 0;
      bits[1][i] = 0;
      bits[2][i] = 0;
      bits[3][i] = 0;
      bits[4][i] = 0;
      bits[5][i] = 0;
      bits[6][i] = 0;
      bits[7][i] = 0;
    }
    //if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
    //    bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
    //    bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
    //    bits[9][i] != 0 || bits[10][i] != 0 || bits[11][i] != 0 ||
    //    bits[12][i] != 0 || bits[13][i] != 0 || bits[14][i] != 0 ||
    //    bits[15][i] != 0)
    //{
    //  countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i], bits[3][i],
    //      bits[4][i], bits[5][i], bits[6][i], bits[7][i], bits[8][i],
    //      bits[9][i], bits[10][i], bits[11][i], bits[12][i], bits[13][i],
    //      bits[14][i], bits[15][i]);
    //}
  }
}

inline void MultiThreadHashCounter::forceFlush(std::atomic_uint32_t* gramCounts)
{
  if (bitsIndex == 0)
    return; // already flushed

  for (size_t i = 0; i < 262144; ++i)
  {
    __builtin_prefetch(&gramCounts[64 * (i + 1)], 1, 3);

    const uint64_t& b = bits[0][i];
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
}

inline void MultiThreadHashCounter::forceFlush(CountsArray<>& countsArray)
{
  if (bitsIndex == 1)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0)
        countsArray.Increment(i, bits[0][i]);
    }
  }
  else if (bitsIndex == 2)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0)
        countsArray.Increment(i, bits[0][i], bits[1][i]);
    }
  }
  else if (bitsIndex == 3)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 4)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i]);
      }
    }
  }
  else if (bitsIndex == 5)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 6)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 7)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 8)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i]);
      }
    }
  }
  else if (bitsIndex == 9)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], (uint64_t) 0, (uint64_t) 0, (uint64_t) 0, (uint64_t) 0,
            (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 10)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
          bits[9][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], bits[9][i], (uint64_t) 0, (uint64_t) 0, (uint64_t) 0,
            (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 11)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
          bits[9][i] != 0 || bits[10][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], bits[9][i], bits[10][i], (uint64_t) 0, (uint64_t) 0,
            (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 12)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
          bits[9][i] != 0 || bits[10][i] != 0 || bits[11][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], bits[9][i], bits[10][i], bits[11][i], (uint64_t) 0,
            (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 13)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
          bits[9][i] != 0 || bits[10][i] != 0 || bits[11][i] != 0 ||
          bits[12][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], bits[9][i], bits[10][i], bits[11][i], bits[12][i],
            (uint64_t) 0, (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 14)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
          bits[9][i] != 0 || bits[10][i] != 0 || bits[11][i] != 0 ||
          bits[12][i] != 0 || bits[13][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], bits[9][i], bits[10][i], bits[11][i], bits[12][i],
            bits[13][i], (uint64_t) 0, (uint64_t) 0);
      }
    }
  }
  else if (bitsIndex == 15)
  {
    for (size_t i = 0; i < 262144; ++i)
    {
      if (bits[0][i] != 0 || bits[1][i] != 0 || bits[2][i] != 0 ||
          bits[3][i] != 0 || bits[4][i] != 0 || bits[5][i] != 0 ||
          bits[6][i] != 0 || bits[7][i] != 0 || bits[8][i] != 0 ||
          bits[9][i] != 0 || bits[10][i] != 0 || bits[11][i] != 0 ||
          bits[12][i] != 0 || bits[13][i] != 0 || bits[14][i] != 0)
      {
        countsArray.Increment(i, bits[0][i], bits[1][i], bits[2][i],
            bits[3][i], bits[4][i], bits[5][i], bits[6][i], bits[7][i],
            bits[8][i], bits[9][i], bits[10][i], bits[11][i], bits[12][i],
            bits[13][i], bits[14][i], (uint64_t) 0);
      }
    }
  }
}

#endif
