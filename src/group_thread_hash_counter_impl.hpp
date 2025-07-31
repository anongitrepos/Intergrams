#ifndef PNGRAM_GROUP_THREAD_HASH_COUNTER_IMPL_HPP
#define PNGRAM_GROUP_THREAD_HASH_COUNTER_IMPL_HPP

#include "group_thread_hash_counter.hpp"
#include <cstring>
#include <iostream>

template<size_t NumBitsets>
inline GroupThreadHashCounter<NumBitsets>::GroupThreadHashCounter(
    CountsArray& counts,
    uint64_t* bitsets,
    std::atomic_uint8_t* bitsetStatuses) :
    countsArray(counts),
    bitsets(bitsets),
    bitsetStatuses(bitsetStatuses),
    waitingForBitset(0),
    waitingForAssignMutex(0),
    flushes(0)
{
  memset(fileIds, 0xFF, sizeof(size_t) * NumBitsets);
}

template<size_t NumBitsets>
inline GroupThreadHashCounter<NumBitsets>::~GroupThreadHashCounter()
{
  std::ostringstream oss;
  oss << "GroupThreadHashCounter<" << NumBitsets << ">: " << waitingForBitset
      << " iterations waiting for a bitset; " << waitingForAssignMutex
      << " iterations waiting for bitset assignment mutex; " << flushes
      << " flushes." << std::endl;
  std::cout << oss.str();
}

template<size_t NumBitsets>
inline size_t GroupThreadHashCounter<NumBitsets>::GetBitsetId(
    const size_t fileId)
{
  // We may have to wait on bitsets to become available.
  do
  {
    // First check if a bitset is already assigned for the file ID.
    bool expected = false;
    while (!assignMutex.compare_exchange_strong(expected, true))
    {
      ++waitingForAssignMutex;
      usleep(10);
    }

    for (size_t b = 0; b < NumBitsets; ++b)
    {
      // The file is already assigned.
      if (fileIds[b] == fileId)
      {
        assignMutex = false;
        return b;
      }
    }

    // If the file is not assigned, find a new location for it if we can.
    for (size_t b = 0; b < NumBitsets; ++b)
    {
      if (bitsetStatuses[b] == 0)
      {
        // Grab this one---it's not assigned.
        bitsetStatuses[b] = 1;
        fileIds[b] = fileId;
        assignMutex = false;
        return b;
      }
    }

    // If we got to here, we did not find a bitset, so release the mutex and
    // wait a little while.  While we're waiting, check to see if some bitsets
    // are "dangling" (i.e. we didn't mark them as done, but they actually are
    // done).
    assignMutex = false;
    ++waitingForBitset;
    usleep(100);
  } while (true);
}

template<size_t NumBitsets>
inline size_t GroupThreadHashCounter<NumBitsets>::GetExistingBitsetId(
    const size_t fileId)
{
  for (size_t b = 0; b < NumBitsets; ++b)
  {
    if (fileIds[b] == fileId)
      return b;
  }

  // This should not happen...
  return size_t(-1);
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Set(
    const unsigned char* b,
    const size_t bitsetId)
{
  const size_t index = ((size_t(*b) << 16) + (size_t(*(b + 1)) << 8) +
      size_t(*(b + 2)));

  const size_t bitLoc = index / 64;
  const size_t bit = index & 0x3F;
  bitsets[262144 * bitsetId + bitLoc] |= (uint64_t(1) << bit);
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::FinishBitset(
    const size_t bitsetId, const bool finish)
{
  // Mark it as finished.
  bitsetStatuses[bitsetId] = 2;
  fileIds[bitsetId] = size_t(-1);

  // If at least half of the bitsets are ready for flushing, we will do that.
  // The count here doesn't need to be exact.
  size_t readyForFlush = 0;
  for (size_t i = 0; i < NumBitsets; ++i)
    if (bitsetStatuses[i] == 2)
      ++readyForFlush;

  if (readyForFlush < 2)
    return;

  // Now actually claim the bitsets.
  size_t indices[NumBitsets];
  for (size_t b = 0; b < NumBitsets; ++b)
    indices[b] = NumBitsets;

  size_t i = 0;
  for (size_t b = 0; b < NumBitsets; ++b)
  {
    uint8_t expected = 2;
    if (bitsetStatuses[b].compare_exchange_strong(expected, 3))
    {
      indices[i] = b;
      ++i;
    }
  }

  // Make sure we got any to flush.
  if (i == 0)
    return;

  // Now call the correct flushing function...
  if (i == 1)
    Flush(indices[0]);
  else if (i == 2)
    Flush(indices[0], indices[1]);
  else if (i == 3)
    Flush(indices[0], indices[1], indices[2]);
  else if (i == 4)
    Flush(indices[0], indices[1], indices[2], indices[3]);
  else if (i == 5)
    Flush(indices[0], indices[1], indices[2], indices[3], indices[4]);
  else if (i == 6)
    Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
        indices[5]);
  else if (i == 7)
    Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
        indices[5], indices[6]);
  else if (i == 8)
    Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
        indices[5], indices[6], indices[7]);
  else if (i > 8)
  {
    Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
        indices[5], indices[6], indices[7]);

    size_t j = 8;
    for ( ; j < i - 8; j += 8)
    {
      Flush(indices[j], indices[j + 1], indices[j + 2], indices[j + 3],
          indices[j + 4], indices[j + 5], indices[j + 6], indices[j + 7]);
    }

    if (j > i)
    {
      // Get the last few.
      if (j - i == 1)
        Flush(indices[i - 1]);
      else if (j - i == 2)
        Flush(indices[i - 2], indices[i - 1]);
      else if (j - i == 3)
        Flush(indices[i - 3], indices[i - 2], indices[i - 1]);
      else if (j - i == 4)
        Flush(indices[i - 4], indices[i - 3], indices[i - 2], indices[i - 1]);
      else if (j - i == 5)
        Flush(indices[i - 5], indices[i - 4], indices[i - 3], indices[i - 2],
            indices[i - 1]);
      else if (j - i == 6)
        Flush(indices[i - 6], indices[i - 5], indices[i - 4], indices[i - 3],
            indices[i - 2], indices[i - 1]);
      else if (j - i == 7)
        Flush(indices[i - 7], indices[i - 6], indices[i - 5], indices[i - 4],
            indices[i - 3], indices[i - 2], indices[i - 1]);
    }
  }

  // Mark bitsets as ready for use or fully finished.
  for (size_t j = 0; j < i; ++j)
    bitsetStatuses[indices[j]] = 0;
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::ForceFlush()
{
  while (true)
  {
    // Gather all of the bitsets we can that need flushing.
    size_t indices[NumBitsets];
    for (size_t b = 0; b < NumBitsets; ++b)
      indices[b] = NumBitsets;

    size_t i = 0;
    for (size_t b = 0; b < NumBitsets; ++b)
    {
      uint8_t expected = 2;
      if (bitsetStatuses[b].compare_exchange_strong(expected, 3))
      {
        indices[i] = b;
        ++i;
      }
    }

    std::cout << "Statuses: [" << (size_t) bitsetStatuses[0];
    for (size_t b = 1; b < NumBitsets; ++b)
      std::cout << ", " << (size_t) bitsetStatuses[b];
    std::cout << "]\n";

    if (i == 0)
      break;

    std::cout << "Force flushing " << i << " bitsets." << std::endl;

    // Flush everything we got.
    if (i == 1)
      Flush(indices[0]);
    else if (i == 2)
      Flush(indices[0], indices[1]);
    else if (i == 3)
      Flush(indices[0], indices[1], indices[2]);
    else if (i == 4)
      Flush(indices[0], indices[1], indices[2], indices[3]);
    else if (i == 5)
      Flush(indices[0], indices[1], indices[2], indices[3], indices[4]);
    else if (i == 6)
      Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
          indices[5]);
    else if (i == 7)
      Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
          indices[5], indices[6]);
    else if (i == 8)
      Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
          indices[5], indices[6], indices[7]);
    else if (i > 8)
    {
      Flush(indices[0], indices[1], indices[2], indices[3], indices[4],
          indices[5], indices[6], indices[7]);

      size_t j = 8;
      for ( ; j < i; j += 8)
      {
        Flush(indices[j], indices[j + 1], indices[j + 2], indices[j + 3],
            indices[j + 4], indices[j + 5], indices[j + 6], indices[j + 7]);
      }
      std::cout << "j: " << j << ", i: " << i << "\n";

      // Get the last few.
      if (j - i == 1)
        Flush(indices[i - 1]);
      else if (j - i == 2)
        Flush(indices[i - 2], indices[i - 1]);
      else if (j - i == 3)
        Flush(indices[i - 3], indices[i - 2], indices[i - 1]);
      else if (j - i == 4)
        Flush(indices[i - 4], indices[i - 3], indices[i - 2], indices[i - 1]);
      else if (j - i == 5)
        Flush(indices[i - 5], indices[i - 4], indices[i - 3], indices[i - 2],
            indices[i - 1]);
      else if (j - i == 6)
        Flush(indices[i - 6], indices[i - 5], indices[i - 4], indices[i - 3],
            indices[i - 2], indices[i - 1]);
      else if (j - i == 7)
        Flush(indices[i - 7], indices[i - 6], indices[i - 5], indices[i - 4],
            indices[i - 3], indices[i - 2], indices[i - 1]);
    }

    // Mark bitsets as ready for use or fully finished.
    for (size_t j = 0; j < i; ++j)
      bitsetStatuses[indices[j]] = 0;
  }

  std::cout << "Finished forced flushing." << std::endl;
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(const size_t index)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index + i]);

      bitsets[262144 * index + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i]);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2, const size_t index3)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0 ||
        bitsets[262144 * index3 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i],
                               bitsets[262144 * index3 + i]);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
      bitsets[262144 * index3 + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2, const size_t index3,
    const size_t index4)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0 ||
        bitsets[262144 * index3 + i] != 0 ||
        bitsets[262144 * index4 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i],
                               bitsets[262144 * index3 + i],
                               bitsets[262144 * index4 + i]);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
      bitsets[262144 * index3 + i] = 0;
      bitsets[262144 * index4 + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2, const size_t index3,
    const size_t index4, const size_t index5)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0 ||
        bitsets[262144 * index3 + i] != 0 ||
        bitsets[262144 * index4 + i] != 0 ||
        bitsets[262144 * index5 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i],
                               bitsets[262144 * index3 + i],
                               bitsets[262144 * index4 + i],
                               bitsets[262144 * index5 + i],
                               (uint64_t) 0,
                               (uint64_t) 0,
                               (uint64_t) 0);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
      bitsets[262144 * index3 + i] = 0;
      bitsets[262144 * index4 + i] = 0;
      bitsets[262144 * index5 + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2, const size_t index3,
    const size_t index4, const size_t index5, const size_t index6)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0 ||
        bitsets[262144 * index3 + i] != 0 ||
        bitsets[262144 * index4 + i] != 0 ||
        bitsets[262144 * index5 + i] != 0 ||
        bitsets[262144 * index6 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i],
                               bitsets[262144 * index3 + i],
                               bitsets[262144 * index4 + i],
                               bitsets[262144 * index5 + i],
                               bitsets[262144 * index6 + i],
                               (uint64_t) 0,
                               (uint64_t) 0);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
      bitsets[262144 * index3 + i] = 0;
      bitsets[262144 * index4 + i] = 0;
      bitsets[262144 * index5 + i] = 0;
      bitsets[262144 * index6 + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2, const size_t index3,
    const size_t index4, const size_t index5, const size_t index6,
    const size_t index7)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0 ||
        bitsets[262144 * index3 + i] != 0 ||
        bitsets[262144 * index4 + i] != 0 ||
        bitsets[262144 * index5 + i] != 0 ||
        bitsets[262144 * index6 + i] != 0 ||
        bitsets[262144 * index7 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i],
                               bitsets[262144 * index3 + i],
                               bitsets[262144 * index4 + i],
                               bitsets[262144 * index5 + i],
                               bitsets[262144 * index6 + i],
                               bitsets[262144 * index7 + i],
                               (uint64_t) 0);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
      bitsets[262144 * index3 + i] = 0;
      bitsets[262144 * index4 + i] = 0;
      bitsets[262144 * index5 + i] = 0;
      bitsets[262144 * index6 + i] = 0;
      bitsets[262144 * index7 + i] = 0;
    }
  }
}

template<size_t NumBitsets>
inline void GroupThreadHashCounter<NumBitsets>::Flush(
    const size_t index1, const size_t index2, const size_t index3,
    const size_t index4, const size_t index5, const size_t index6,
    const size_t index7, const size_t index8)
{
  ++flushes;

  for (size_t i = 0; i < 262144; ++i)
  {
    if (bitsets[262144 * index1 + i] != 0 ||
        bitsets[262144 * index2 + i] != 0 ||
        bitsets[262144 * index3 + i] != 0 ||
        bitsets[262144 * index4 + i] != 0 ||
        bitsets[262144 * index5 + i] != 0 ||
        bitsets[262144 * index6 + i] != 0 ||
        bitsets[262144 * index7 + i] != 0 ||
        bitsets[262144 * index8 + i] != 0)
    {
      countsArray.Increment(i, bitsets[262144 * index1 + i],
                               bitsets[262144 * index2 + i],
                               bitsets[262144 * index3 + i],
                               bitsets[262144 * index4 + i],
                               bitsets[262144 * index5 + i],
                               bitsets[262144 * index6 + i],
                               bitsets[262144 * index7 + i],
                               bitsets[262144 * index8 + i]);

      bitsets[262144 * index1 + i] = 0;
      bitsets[262144 * index2 + i] = 0;
      bitsets[262144 * index3 + i] = 0;
      bitsets[262144 * index4 + i] = 0;
      bitsets[262144 * index5 + i] = 0;
      bitsets[262144 * index6 + i] = 0;
      bitsets[262144 * index7 + i] = 0;
      bitsets[262144 * index8 + i] = 0;
    }
  }
}

#endif
