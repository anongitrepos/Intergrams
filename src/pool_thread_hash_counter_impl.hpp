#ifndef PNGRAM_POOL_THREAD_HASH_COUNTER_IMPL_HPP
#define PNGRAM_POOL_THREAD_HASH_COUNTER_IMPL_HPP

#include "pool_thread_hash_counter.hpp"
#include <cstring>
#include <iostream>

template<size_t C>
inline PoolThreadHashCounter<C>::PoolThreadHashCounter(
    uint64_t** bitsets,
    std::atomic_uint8_t* bitsetStatuses) :
    bitsets(bitsets),
    bitsetStatuses(bitsetStatuses),
    bitsIndex(0),
    waitingForBitset(0),
    flushes(0)
{
  claim();
}

template<size_t C>
inline void PoolThreadHashCounter<C>::claim()
{
  size_t index = 0;
  bool found = false;
  //std::ostringstream oss;
  //oss << "claim with statuses [";
  //for (size_t i = 0; i < C - 1; ++i)
  //  oss << (size_t) bitsetStatuses[i] << ", ";
  //oss << (size_t) bitsetStatuses[C - 1] << "]\n";
  //std::cout << oss.str();
  while (!found)
  {
    bool finished = false;
    do
    {
      finished = true;
      uint8_t expected = 0;
      if (bitsetStatuses[index].compare_exchange_strong(expected, 1))
      {
        // We grabbed the bitset.
        bitsIndex = index;

        // Prefetch it...
        for (size_t i = 0; i < 262144; i += 16)
          __builtin_prefetch(&bitsets[bitsIndex][i], 1, 3);

        return;
      }

      if (expected != 4)
        finished = false;

      index++;
      if (index >= C)
        index = 0;
    } while (index != bitsIndex);

    if (finished)
      return; // nothing left to do

    ++waitingForBitset;
    usleep(1000);
  }
}

template<size_t C>
inline void PoolThreadHashCounter<C>::set(const unsigned char* b)
{
  const size_t index = ((size_t(*b) << 16) + (size_t(*(b + 1)) << 8) +
      size_t(*(b + 2)));

  const size_t bitLoc = index / 64;
  const size_t bit = index & 0x3F;
  bitsets[bitsIndex][bitLoc] |= (uint64_t(1) << bit);
}

template<size_t C>
inline void PoolThreadHashCounter<C>::flush(CountsArray& countsArray)
{
  // See if we can find four bitsets that need to be flushed.
  bitsetStatuses[bitsIndex] = 2;
  size_t indices[8] = { C, C, C, C, C, C, C, C };
  size_t indicesIndex = 0;
  size_t index = bitsIndex;
  //std::ostringstream oss2;
  //oss2 << "Flush with bitset statuses: [";
  //for (size_t i = 0; i < C - 1; ++i)
  //  oss2 << (size_t) bitsetStatuses[i] << ", ";
  //oss2 << (size_t) bitsetStatuses[C - 1] << "]\n";
  //std::cout << oss2.str();

  do
  {
    uint8_t expected = 2;
    if (bitsetStatuses[index].compare_exchange_strong(expected, 3))
    {
      indices[indicesIndex] = index;
      ++indicesIndex;
      if (indicesIndex == 8)
        break;
    }

    index++;
    if (index >= C)
      index = 0;
  } while (index != bitsIndex);

  if (indicesIndex != 8)
  {
    // Unreserve the ones we grabbed---we don't have enough for a flush.
    for (size_t i = 0; i < indicesIndex; ++i)
      bitsetStatuses[indices[i]] = 2;

    return;
  }

  //std::ostringstream oss;
  //oss << "Flush indices " << indices[0] << ", " << indices[1] << ", " << indices[2] << ", " << indices[3] << "\n";
  //std::cout << oss.str();

  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[indices[0]][i] != 0 || bitsets[indices[1]][i] != 0 ||
        bitsets[indices[2]][i] != 0 || bitsets[indices[3]][i] != 0 ||
        bitsets[indices[4]][i] != 0 || bitsets[indices[5]][i] != 0 ||
        bitsets[indices[6]][i] != 0 || bitsets[indices[7]][i] != 0)
    {
      countsArray.Increment(i, bitsets[indices[0]][i], bitsets[indices[1]][i],
          bitsets[indices[2]][i], bitsets[indices[3]][i],
          bitsets[indices[4]][i], bitsets[indices[5]][i],
          bitsets[indices[6]][i], bitsets[indices[7]][i]);

      bitsets[indices[0]][i] = 0;
      bitsets[indices[1]][i] = 0;
      bitsets[indices[2]][i] = 0;
      bitsets[indices[3]][i] = 0;
      bitsets[indices[4]][i] = 0;
      bitsets[indices[5]][i] = 0;
      bitsets[indices[6]][i] = 0;
      bitsets[indices[7]][i] = 0;
    }
  }

  // We are done with these bitsets, so they can be used again.
  bitsetStatuses[indices[0]] = 0;
  bitsetStatuses[indices[1]] = 0;
  bitsetStatuses[indices[2]] = 0;
  bitsetStatuses[indices[3]] = 0;
  bitsetStatuses[indices[4]] = 0;
  bitsetStatuses[indices[5]] = 0;
  bitsetStatuses[indices[6]] = 0;
  bitsetStatuses[indices[7]] = 0;
}

template<size_t C>
inline void PoolThreadHashCounter<C>::forceFlush(CountsArray& countsArray)
{
  // Try to grab up to four bitsets to flush at a time, until we don't get any
  // more.
  while (true)
  {
    size_t indices[4] = { C, C, C, C };
    size_t indicesIndex = 0;
    size_t index = bitsIndex;
    do
    {
      uint8_t expected = 2;
      if (bitsetStatuses[index].compare_exchange_strong(expected, 3))
      {
        indices[indicesIndex] = index;
        ++indicesIndex;
        if (indicesIndex == 4)
          break;
      }

      index++;
      if (index >= C)
        index = 0;
    } while (index != bitsIndex);

    // We didn't find anything to flush.
    if (indicesIndex == 0)
      return;

    ++flushes;

    if (indicesIndex == 1)
    {
      for (size_t i = 0; i < 262144; ++i)
      {
        if (bitsets[indices[0]][i] != 0)
          countsArray.Increment(i, bitsets[indices[0]][i]);
      }

      bitsetStatuses[indices[0]] = 4;
    }
    else if (indicesIndex == 2)
    {
      for (size_t i = 0; i < 262144; ++i)
      {
        if (bitsets[indices[0]][i] != 0 || bitsets[indices[1]][i] != 0)
        {
          countsArray.Increment(i, bitsets[indices[0]][i],
              bitsets[indices[1]][i]);
        }
      }

      bitsetStatuses[indices[0]] = 4;
      bitsetStatuses[indices[1]] = 4;
    }
    else if (indicesIndex == 3)
    {
      for (size_t i = 0; i < 262144; ++i)
      {
        if (bitsets[indices[0]][i] != 0 || bitsets[indices[1]][i] != 0 ||
            bitsets[indices[2]][i] != 0)
        {
          countsArray.Increment(i, bitsets[indices[0]][i],
              bitsets[indices[1]][i], bitsets[indices[2]][i]);
        }
      }

      bitsetStatuses[indices[0]] = 4;
      bitsetStatuses[indices[1]] = 4;
      bitsetStatuses[indices[2]] = 4;
    }
    else
    {
      for (size_t i = 0; i < 262144; ++i)
      {
        if (bitsets[indices[0]][i] != 0 || bitsets[indices[1]][i] != 0 ||
            bitsets[indices[2]][i] != 0 || bitsets[indices[3]][i] != 0)
        {
          countsArray.Increment(i, bitsets[indices[0]][i],
              bitsets[indices[1]][i], bitsets[indices[2]][i],
              bitsets[indices[3]][i]);
        }
      }

      bitsetStatuses[indices[0]] = 4;
      bitsetStatuses[indices[1]] = 4;
      bitsetStatuses[indices[2]] = 4;
      bitsetStatuses[indices[3]] = 4;
    }
  }
}

#endif
