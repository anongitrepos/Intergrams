#ifndef PNGRAM_NGRAM_LOCKSTEP_THREAD_IMPL_HPP
#define PNGRAM_NGRAM_LOCKSTEP_THREAD_IMPL_HPP

#include "ngram_lockstep_thread.hpp"
#include <cstring>
#include <armadillo>
#include <sched.h>
#include <pthread.h>
#include <immintrin.h>

template<typename ReaderThreadType>
inline NgramLockstepThread<ReaderThreadType>::NgramLockstepThread(
    ReaderThreadType& reader,
    const size_t threadId,
    uint32_t* gram3Counts,
    std::atomic_uint64_t& globalWaitingForChunk,
    std::atomic<double>& globalFlushTime) :
    reader(reader),
    threadId(threadId),
    // We care about the first byte having the first six bits equivalent to
    // threadId.
    mask(uint8_t(threadId) << 2),
    gram3Counts(gram3Counts),
    gram3Offset(threadId * N),
    mask8Bytes(uint64_t(mask) +
               (uint64_t(mask) << 8) +
               (uint64_t(mask) << 16) +
               (uint64_t(mask) << 24) +
               (uint64_t(mask) << 32) +
               (uint64_t(mask) << 40) +
               (uint64_t(mask) << 48) +
               (uint64_t(mask) << 54)),
    fullMask { mask8Bytes, mask8Bytes, mask8Bytes, mask8Bytes,
               mask8Bytes, mask8Bytes, mask8Bytes, mask8Bytes },
    seqMasks { uint64_t(mask) << 54,
               uint64_t(mask) << 48,
               uint64_t(mask) << 40,
               uint64_t(mask) << 32,
               uint64_t(mask) << 24,
               uint64_t(mask) << 16,
               uint64_t(mask) << 8,
               uint64_t(mask) },
    thread(&NgramLockstepThread::RunThread, this),
    waitingForChunk(0),
    flushTime(0),
    globalWaitingForChunk(globalWaitingForChunk),
    globalFlushTime(globalFlushTime)
{
  // Try to set thread affinity.
  pthread_t nativeThread = thread.native_handle();
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(threadId, &cpuset);
  int rc = pthread_setaffinity_np(nativeThread,
                                  sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
  }

  // Try to set thread priority.
  sched_param sp { 99 };
  pthread_setschedparam(nativeThread, SCHED_FIFO, &sp);
}

template<typename ReaderThreadType>
inline NgramLockstepThread<ReaderThreadType>::~NgramLockstepThread()
{
  if (thread.joinable())
    thread.join();

  globalWaitingForChunk += waitingForChunk;
  globalFlushTime += flushTime;
}

template<typename ReaderThreadType>
inline NgramLockstepThread<ReaderThreadType>::NgramLockstepThread(
    NgramLockstepThread&& other) noexcept :
    reader(other.reader),
    threadId(other.threadId),
    mask(other.mask),
    gram3Counts(other.gram3Counts),
    gram3Offset(other.gram3Offset),
    thread(std::move(other.thread)),
    globalWaitingForChunk(other.globalWaitingForChunk),
    globalFlushTime(other.globalFlushTime)
{
  // Copy bits.
  memcpy(bits, other.bits, sizeof(uint8_t) * B);
}

template<typename ReaderThreadType>
inline void NgramLockstepThread<ReaderThreadType>::RunThread()
{
  // Clear bits.
  memset(bits, 0, sizeof(uint8_t) * B);
  //arma::wall_clock c;

  // Main loop: grab chunks and process them.
  bool done = false;
  size_t lastFileId = 0;
  size_t fileId = 0;
  u512* ptr = nullptr;
  size_t bytes = 0;
  size_t chunkId = size_t(-1);
  while (reader.GetNextChunk(ptr, bytes, fileId, chunkId, threadId))
  {
    //std::ostringstream oss;
    //oss << "Thread " << threadId << " got chunk id " << chunkId << " with " << bytes << " bytes for file " << fileId << " (last " << lastFileId << ")." << std::endl;
    //std::cout << oss.str();

    // First, do we have to flush what we have?
    if (lastFileId != fileId)
    {
      //c.tic();
      // Iterate over our bits.
      for (size_t i = 0; i < B; ++i)
      {
        uint8_t& bit = bits[i];
        const size_t fullOffset = gram3Offset + i * 8;
        if (bit > 0)
        {
          //std::cout << "flush to gram3Counts\n";
          gram3Counts[fullOffset] += uint32_t(bit & 0x01);
          gram3Counts[fullOffset + 1] += uint32_t((bit & 0x02) >> 1);
          gram3Counts[fullOffset + 2] += uint32_t((bit & 0x04) >> 2);
          gram3Counts[fullOffset + 3] += uint32_t((bit & 0x08) >> 3);
          gram3Counts[fullOffset + 4] += uint32_t((bit & 0x10) >> 4);
          gram3Counts[fullOffset + 5] += uint32_t((bit & 0x20) >> 5);
          gram3Counts[fullOffset + 6] += uint32_t((bit & 0x40) >> 6);
          gram3Counts[fullOffset + 7] += uint32_t((bit & 0x80) >> 7);
        }
        bit = 0;
      }

      //memset(bits, 0, sizeof(uint8_t) * B);
      //std::ostringstream oss;
      //oss << "Finished flush for file " << lastFileId << ", thread " << threadId << ".\n";
      //std::cout << oss.str();
      lastFileId = fileId;
      //flushTime += c.toc();
    }

    // Second, did we get anything?
    if (ptr == nullptr)
    {
      //std::ostringstream oss;
      //oss << "Thread " << threadId << " must wait on chunk " << chunkId << "!\n";
      //std::cout << oss.str();
      ++waitingForChunk;
      usleep(100);
      continue;
    }

    // Now, process the bytes we got.
    ProcessChunkSIMD(ptr, bytes, bits);

    reader.FinishChunk(chunkId, threadId);
  }

  // If we're done, we have to flush the last file.
  for (size_t i = 0; i < B; ++i)
    if (bits[i] != 0)
      for (size_t b = 0; b < 8; ++b)
        if ((bits[i] & (1 << b)) != 0)
          ++gram3Counts[gram3Offset + i * 8 + b];
}

template<typename ReaderThreadType>
inline void NgramLockstepThread<ReaderThreadType>::ProcessChunkSIMD(
    const u512* ptr, const size_t bytes, uint8_t* targetBits)
{
  if (bytes < 3)
    return;

  // Process in 64-byte chunks using SIMD.
  const size_t maxOuter = (bytes - 2) / 64;
  for (size_t i = 0; i < maxOuter; ++i)
  {
    // For each of 64 bytes, we care about whether the top 6 bits match our
    // thread id.  So, given a byte, we can mask off the bottom 2 bits, and then
    // XOR the top 6 bits with our desired 6 bits.  This will return a byte of
    // 0x00 when there is a matching 6-bit prefix.
    //
    // Then I use a clever bitwise trick to turn that 0x00 into a nonzero value
    // and any nonzero value into a 0x00.  All of this is done in service of
    // checking if there are any bytes in this 64-byte sequence that this thread
    // cares about.
    uint64_t maxVal = 0;
    u512 test = (ptr[i] & prefixMask) ^ fullMask;
    test = (test - lowBitSet) & (~test & highBitSet);
    for (size_t j = 0; j < 8; ++j)
      maxVal |= test[j];

    if (maxVal == 0)
      continue;

    // Next, we have to process this 64-byte chunk.  So, do this in 8-byte
    // chunks.
    for (size_t j = 0; j < 8; ++j)
    {
      // Before doing this, check if this 8-byte chunk has anything we care
      // about.
      if (((ptr[i])[j] & mask8Bytes) == 0)
        continue;

      // Make 8 copies of the 8-byte sequence.
      u512 dataChunk = __builtin_shuffle(ptr[i],
          i512 { (int64_t) j, (int64_t) j, (int64_t) j, (int64_t) j,
                 (int64_t) j, (int64_t) j, (int64_t) j, (int64_t) j });
      u512 nextDataChunk = (j == 7) ?
          __builtin_shuffle(ptr[i + 1], i512 { 0, 0, 0, 0, 0, 0, 0, 0 }) :
          __builtin_shuffle(ptr[i],
              i512 { (int64_t) j + 1, (int64_t) j + 1,
                     (int64_t) j + 1, (int64_t) j + 1,
                     (int64_t) j + 1, (int64_t) j + 1,
                     (int64_t) j + 1, (int64_t) j + 1 });

      // Compute the 3-gram sequences that could matter.
      u512 seqMask = (dataChunk & seqMasks);
      for (size_t k = 0; k < 8; ++k)
        seqMask[k] = (seqMask[k] != 0) ? 0xFFFFFFFFFFFFFFFF : 0;

      // Compute byte indexes for each 3-byte sequence starting at each of the
      // offsets (0 through 7 bytes).
      u512 byteIndexes = (((dataChunk & byteIndexMasks)
          >> byteIndexShifts)) +
          ((nextDataChunk & nextByteIndexMasks) >> rightNextByteIndexShifts);

      // Compute the actual values that we would need to add to the bytes in
      // `bits` for each 3-byte sequence.
      u512 bitValuesVec = ((1 << (((dataChunk & bitValueMasks)
          >> bitValueShifts)) +
          (nextDataChunk & nextBitValueMasks) >> rightNextBitValueShifts)
          << finalBitValueShifts) & seqMask;

      uint64_t bitValues = 0;
      for (size_t k = 0; k < 8; ++k)
        bitValues += bitValuesVec[k];

      // Last sanity check before we really do work: if there are no
      // values in the first 4 byte sequences, skip them.
      if (bitValues >= 0x0000000100000000)
      {
        targetBits[byteIndexes[0]] |= ((bitValues & 0xFF00000000000000) >> 54);
        targetBits[byteIndexes[1]] |= ((bitValues & 0x00FF000000000000) >> 48);
        targetBits[byteIndexes[2]] |= ((bitValues & 0x0000FF0000000000) >> 40);
        targetBits[byteIndexes[3]] |= ((bitValues & 0x000000FF00000000) >> 32);
      }

      // And similarly for the last 4 byte sequences.
      if ((bitValues & 0x00000000FFFFFFFF) > 0)
      {
        targetBits[byteIndexes[4]] |= ((bitValues & 0x00000000FF000000) >> 24);
        targetBits[byteIndexes[5]] |= ((bitValues & 0x0000000000FF0000) >> 16);
        targetBits[byteIndexes[6]] |= ((bitValues & 0x000000000000FF00) >>  8);
        targetBits[byteIndexes[7]] |= ((bitValues & 0x00000000000000FF));
      }
    }
  }

  // TODO: handle the last bits
}

#endif
