// lockstep_reader_thread_impl.hpp: implementation of LockstepReaderThread
// functionality.
#ifndef PNGRAM_LOCKSTEP_READER_THREAD_IMPL_HPP
#define PNGRAM_LOCKSTEP_READER_THREAD_IMPL_HPP

#include "lockstep_reader_thread.hpp"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

inline LockstepReaderThread::LockstepReaderThread(
    std::filesystem::path directory,
    const size_t n) :
    dirIter(std::vector<std::filesystem::path>{ directory }, false),
    localBuffer(nullptr),
    nextChunk(0),
    n(n),
    waitingForChunks(0),
    thread(&LockstepReaderThread::RunThread, this),
    finished(false)
{
  // The thread has now started.
}

inline LockstepReaderThread::~LockstepReaderThread()
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
        std::ostringstream oss;
        oss << "Waiting on chunk " << i << ": " << chunkDoneCount[i] << "\n";
        std::cout << oss.str();
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

inline void LockstepReaderThread::RunThread()
{
  alloc_hugepage<u512>(localBuffer, localBufferMemState, totalBufferSize / 64,
      "n-gramming");
  memset(localBuffer, 0, sizeof(u512) * totalBufferSize / 64);

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

inline bool LockstepReaderThread::GetNextChunk(u512*& ptr,
                                               size_t& bytes,
                                               size_t& fileId,
                                               size_t& chunkId,
                                               const size_t /* threadId */)
{
  // Increment the chunk id.
  const size_t lastChunkId = chunkId;
  chunkId++;
  if (chunkId >= 2 * numChunks)
    chunkId = 0;

  // If we are looking at the same place as the next chunk to be filled, we know
  // we need to wait.
  if (chunkId == nextChunk)
  {
    ptr = nullptr;
    bytes = 0;
    chunkId = lastChunkId;
    return !finished;
  }

  // If we aren't looking at the same place as the next chunk to be filled, then
  // we can take the chunk if there is a need.
  const size_t actualChunkId = (chunkId >= numChunks) ? chunkId - numChunks :
      chunkId;

  if (chunkDoneCount[actualChunkId] > 0)
  {
    ptr = localBuffer + actualChunkId * chunkStep;
    bytes = chunkSizes[actualChunkId];
    fileId = chunkFileIds[actualChunkId];
    return true;
  }
  else
  {
    // We are waiting to prepare chunk ID nextChunk.
    ptr = nullptr;
    bytes = 0;
    chunkId = lastChunkId;
    return !finished;
  }
}

inline void LockstepReaderThread::FinishChunk(const size_t chunkId,
                                              const size_t threadId)
{
  const size_t actualChunkId = (chunkId >= numChunks) ? (chunkId - numChunks) :
      chunkId;
  const size_t newCount = (size_t) --chunkDoneCount[actualChunkId];
}

#endif
