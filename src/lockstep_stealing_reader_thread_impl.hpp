// lockstep_stealing_reader_thread_impl.hpp: implementation of
// LockstepStealingReaderThread functionality.
#ifndef PNGRAM_LOCKSTEP_STEALING_READER_THREAD_IMPL_HPP
#define PNGRAM_LOCKSTEP_STEALING_READER_THREAD_IMPL_HPP

#include "lockstep_stealing_reader_thread.hpp"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

inline LockstepStealingReaderThread::LockstepStealingReaderThread(
    std::filesystem::path directory,
    const size_t n) :
    dirIter(std::vector<std::filesystem::path>{ directory }, false),
    localBuffer(nullptr),
    nextChunk(0),
    n(n),
    waitingForChunks(0),
    thread(&LockstepStealingReaderThread::RunThread, this),
    finished(false)
{
  // The thread has now started.
}

inline LockstepStealingReaderThread::~LockstepStealingReaderThread()
{
  if (thread.joinable())
    thread.join();

  bool done = false;
  while (!done)
  {
    done = true;
    for (size_t i = 0; i < numChunks; ++i)
    {
      if (chunkDoneCount[i] > 0)
      {
        done = false;
        //std::ostringstream oss;
        //oss << "Waiting on chunk " << i << ": " << chunkDoneCount[i] << "\n";
        //std::cout << oss.str();
        break;
      }
    }

    usleep(1000);
  }

  std::ostringstream oss;
  oss << "ReaderThread statistics:\n";
  oss << " - " << waitingForChunks << " iterations waiting on a chunk to be available.\n";
  std::cout << oss.str();

  free_hugepage<u512>(localBuffer, localBufferMemState, totalBufferSize / 64);
}

inline void LockstepStealingReaderThread::RunThread()
{
  alloc_hugepage<u512>(localBuffer, localBufferMemState, totalBufferSize / 64,
      "n-gramming");
  memset(localBuffer, 0, sizeof(u512) * totalBufferSize / 64);

  memset(lastClaimedChunk, 0xFF, sizeof(std::atomic_uint16_t) * 64);
  memset(activeChunksStolen, 0x00, sizeof(std::atomic_uint16_t) * 64);
  memset(assigningChunk, 0, sizeof(std::atomic_bool) * 64);

  if (localBuffer == nullptr)
    throw std::invalid_argument("could not allocate local buffer");

  // mark all chunks unclaimed and unavailable
  memset(chunkDoneCount, 0, sizeof(std::atomic_uint8_t) * numChunks);

  std::filesystem::path p;
  size_t i;
  while (dirIter.get_next(p, i))
  {
    if (i % 1000 == 0)
      std::cout << "Reading file " << i << "..." << std::endl;

    int fd = open(p.c_str(), O_RDONLY); // TODO: O_DIRECT
    if (fd == -1)
    {
      // check errno, something went wrong
      std::cout << "open failed! errno " << errno << "\n";
      continue;
    }

    // read into buffer
    ssize_t bytesRead = 0;
    do
    {
      const size_t actualNextChunk = (nextChunk >= numChunks) ?
          (nextChunk - numChunks) : nextChunk;

      // Find the chunk we will read into.  There are cases in which we will
      // need to wait...
      while (chunkDoneCount[actualNextChunk] > 0)
      {
        // Still waiting on a thread to finish...
        ++waitingForChunks;
        //std::ostringstream oss;
        //oss << "Cannot refill chunk " << actualNextChunk << ", waiting on " << (size_t) chunkDoneCount[actualNextChunk] << " threads...\n";
        //std::cout << oss.str();
        usleep(100);
      }

      bytesRead = read(fd, localBuffer + actualNextChunk * chunkStep, chunkSize);
      if (bytesRead > 0)
      {
        // Prep for reading by chunks.
        chunkFileIds[actualNextChunk] = i;
        chunkSizes[actualNextChunk] = bytesRead;
        chunkDoneCount[actualNextChunk] = 64;
        ++nextChunk;
        if (nextChunk >= 2 * numChunks)
          nextChunk = 0;

        // rewind for next read
        off_t o = -(n - 1);
        if (lseek(fd, -o, SEEK_CUR) == -1)
          std::cout << "lseek fail!\n";
      }
      else if (bytesRead == -1)
      {
        // error
        std::cout << "read failed! errno " << errno << "\n";
      }
    } while (bytesRead > 0);

    //std::cout << "Done with file " << i << ".\n";

    close(fd);
  }

  finished = true;
}

inline bool LockstepStealingReaderThread::GetNextChunk(u512*& ptr,
                                                       size_t& bytes,
                                                       size_t& fileId,
                                                       size_t& chunkId,
                                                       const size_t threadId)
{
  // Lockout check: if we are stealing a chunk for this thread, come back later.
  bool expected = false;
  if (!assigningChunk[threadId].compare_exchange_strong(expected, 1))
  {
    //std::ostringstream oss;
    //oss << "can't assign to thread " << threadId << ", locked!\n";
    //std::cout << oss.str();
    ptr = nullptr;
    bytes = 0;
    return true;
  }

  // Now we have 'locked' the chunk assignment for this thread.

  // Increment the chunk id, possibly multiple steps.
  const size_t lastChunkId = chunkId;
  chunkId = lastClaimedChunk[threadId] + 1;
  if (chunkId >= 2 * numChunks)
    chunkId = 0;

  // If we are looking at the same place as the next chunk to be filled, we know
  // we need to wait.
  if (chunkId == nextChunk)
  {
    ptr = nullptr;
    bytes = 0;
    chunkId = lastChunkId;
    assigningChunk[threadId] = false;
    if (finished && activeChunksStolen[threadId] > 0)
    {
      std::ostringstream oss;
      oss << "letting thread " << threadId << " finish even though " << activeChunksStolen[threadId] << " chunks are stolen and active\n";
      std::cout << oss.str();
    }
    return !finished;
  }

  // If we aren't looking at the same place as the next chunk to be filled, then
  // we can take the chunk if there is a need.
  const size_t actualChunkId = (chunkId >= numChunks) ? chunkId - numChunks :
      chunkId;

  if (chunkDoneCount[actualChunkId] > 0)
  {
    // Double check: we cannot advance if we are still waiting on a stolen chunk
    // and there was a file change.
    if (activeChunksStolen[threadId] > 0 &&
        chunkFileIds[actualChunkId] != fileId)
    {
      //std::ostringstream oss;
      //oss << "can't assign thread " << threadId << " because we are at a file boundary and " << activeChunksStolen[threadId] << " chunks are stolen\n";
      //std::cout << oss.str();
      ptr = nullptr;
      bytes = 0;
      chunkId = lastChunkId;
      assigningChunk[threadId] = false;
      return true; // wait and come back...
    }

    ptr = localBuffer + actualChunkId * chunkStep;
    bytes = chunkSizes[actualChunkId];
    fileId = chunkFileIds[actualChunkId];
    lastClaimedChunk[threadId] = chunkId;
    assigningChunk[threadId] = false;
    //std::ostringstream oss;
    //oss << "assign chunk " << chunkId << " for thread " << threadId << "\n";
    //std::cout << oss.str();
    return true;
  }
  else
  {
    // We are waiting to prepare chunk ID nextChunk.
    ptr = nullptr;
    bytes = 0;
    //std::ostringstream oss;
    //oss << "waiting on chunk " << actualChunkId << " to be ready for thread " << threadId << "\n";
    //std::cout << oss.str();
    chunkId = lastChunkId;
    // Make sure to wait if we are done but there are still active chunks
    // stolen.
    assigningChunk[threadId] = false;
    return (!finished || (activeChunksStolen[threadId] > 0));
  }
}

inline void LockstepStealingReaderThread::FinishChunk(const size_t chunkId,
                                                      const size_t threadId)
{
  const size_t actualChunkId = (chunkId >= numChunks) ? (chunkId - numChunks) :
      chunkId;
  const size_t newCount = (size_t) --chunkDoneCount[actualChunkId];
  //std::ostringstream oss;
  //oss << "chunk ID " << chunkId << " done for thread " << threadId << "\n";
  //std::cout << oss.str();
}

inline bool LockstepStealingReaderThread::StealNextChunk(
    u512*& ptr,
    size_t& bytes,
    size_t& fileId,
    size_t& chunkId,
    size_t& threadId)
{
  // Determine if there are any chunks to be stolen, by finding the thread that
  // has fallen the most behind.
  size_t biggestGap = 0;
  size_t worstThreadId = 0;
  const size_t actualNextChunk = (nextChunk >= numChunks) ?
      nextChunk - numChunks : nextChunk;
  for (size_t t = 0; t < 64; ++t)
  {
    size_t tryThreadId = threadId + t;
    if (tryThreadId >= 64)
      tryThreadId -= 64;

    size_t lastClaimed = lastClaimedChunk[tryThreadId];
    if (lastClaimed >= numChunks)
      lastClaimed -= numChunks;
    const size_t gap = (actualNextChunk > lastClaimed) ?
        (actualNextChunk - lastClaimed) :
        (numChunks - (lastClaimed - actualNextChunk));

    if (gap > biggestGap)
    {
      biggestGap = gap;
      worstThreadId = tryThreadId;
    }
  }

  // If we've fallen more than 32 chunks behind (arbitrary choice of threshold
  // there), steal the chunk.
  if (biggestGap > 32)
  {
    // Lockout check: if another thread is trying to be assigned a chunk from
    // this thread, then we just don't do anything.
    bool expected = false;
    if (!assigningChunk[worstThreadId].compare_exchange_strong(expected, 1))
    {
      ptr = nullptr;
      bytes = 0;
      threadId = worstThreadId;
      return true /* don't terminate */;
    }

    // Now we have locked out other threads from assigning chunks for this
    // thread.
    ++activeChunksStolen[worstThreadId];
    chunkId = lastClaimedChunk[worstThreadId] + 1;
    if (chunkId >= 2 * numChunks)
      chunkId = 0;
    lastClaimedChunk[worstThreadId] = chunkId;
    const size_t actualChunkId = (chunkId >= numChunks) ?
        (chunkId - numChunks) : chunkId;
    ptr = localBuffer + actualChunkId * chunkStep;
    bytes = chunkSizes[actualChunkId];
    fileId = chunkFileIds[actualChunkId];
    threadId = worstThreadId;
    assigningChunk[worstThreadId] = false;
    //std::ostringstream oss;
    //oss << "steal chunk " << actualChunkId << " for thread " << threadId << " size " << bytes << "; gap " << biggestGap << "\n";
    //std::cout << oss.str();
    return true /* don't terminate */;
  }
  else
  {
    // Nothing to steal right now, come again later.
    ptr = nullptr;
    bytes = 0;
    return !finished;
  }
}

inline void LockstepStealingReaderThread::FinishStolenChunk(
    const size_t chunkId,
    const size_t threadId)
{
  const size_t actualChunkId = (chunkId >= numChunks) ?
      (chunkId - numChunks) : chunkId;

  --activeChunksStolen[threadId];
  --chunkDoneCount[actualChunkId];
}

#endif
