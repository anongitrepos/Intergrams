#ifndef WORK_STEALER_THREAD_IMPL_HPP
#define WORK_STEALER_THREAD_IMPL_HPP

#include "work_stealer_thread.hpp"
#include <cstring>

inline WorkStealerThread::WorkStealerThread(
    LockstepStealingReaderThread& reader,
    std::vector<NgramLockstepThread<LockstepStealingReaderThread>*>& ngramThreads) :
    reader(reader),
    ngramThreads(ngramThreads),
    waitingForChunkToSteal(0),
    thread(&WorkStealerThread::RunThread, this)
{
  // Nothing to do.
}

inline WorkStealerThread::~WorkStealerThread()
{
  if (thread.joinable())
    thread.join();
}

inline void WorkStealerThread::RunThread()
{
  // On initialization, set all the memory to zeros.
  memset(bits, 0, sizeof(uint8_t) * B);

  u512* ptr = nullptr;
  size_t bytes = 0;
  size_t lastFileId = size_t(-1);
  size_t fileId = 0;
  size_t chunkId = 0;
  size_t lastThreadId = 0;
  size_t threadId = 0;
  while (reader.StealNextChunk(ptr, bytes, fileId, chunkId, threadId))
  {
    // Did we get anything?
    if (ptr == nullptr)
    {
      // We aren't done, but we didn't get anything to steal.  Keep waiting.
      ++waitingForChunkToSteal;
      usleep(1000);
      continue;
    }

    // Did we change from the last file we were looking at?
    // If so, we need to flush our last bits back to the thread's bits.
    if (fileId != lastFileId && lastFileId != size_t(-1))
    {
      // TODO: this could be a synchronization nightmare
      for (size_t i = 0; i < B; ++i)
        ngramThreads[lastThreadId]->bits[i] |= bits[i];

      memset(bits, 0, sizeof(uint8_t) * B);
    }

    if (threadId != lastThreadId && lastThreadId != size_t(-1))
    {
      // TODO: this could be a synchronization nightmare
      for (size_t i = 0; i < B; ++i)
        ngramThreads[lastThreadId]->bits[i] |= bits[i];

      memset(bits, 0, sizeof(uint8_t) * B);
    }

    // We got a chunk to process.
    ngramThreads[threadId]->ProcessChunkSIMD(ptr, bytes, bits);
    reader.FinishStolenChunk(chunkId, threadId);
  }
}
#endif
