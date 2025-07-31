// pool_ngram_thread_impl.hpp: implementation of PoolNgramThread
#ifndef PNGRAM_POOL_NGRAM_THREAD_IMPL_HPP
#define PNGRAM_POOL_NGRAM_THREAD_IMPL_HPP

#include "pool_ngram_thread.hpp"

#include <unistd.h>
#include <iostream>
#include <sys/sysinfo.h>
#include <sched.h>
#include <pthread.h>

template<size_t C>
inline PoolNgramThread<C>::PoolNgramThread(SingleReaderThread& reader,
                                           CountsArray& globalCounts,
                                           uint64_t** bitsets,
                                           std::atomic_uint8_t* bitsetStatuses,
                                           const size_t n,
                                           const size_t t) :
  reader(reader),
  threadCounter(bitsets, bitsetStatuses),
  globalCounts(globalCounts),
  n(n),
  waitingForData(0),
  processTime(0.0),
  flushTime(0.0),
  flushCount(0),
  t(t),
  thread(&PoolNgramThread::RunThread, this)
{
  // The thread has now started.  Set its affinity to a physical CPU (this is
  // specific to the uberservers which have 128 processors...).
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  const size_t numCores = get_nprocs();
  const size_t start1 = (t % 4) * 16;
  const size_t end1 = ((t % 4) + 1) * 16 - 1;
  const size_t start2 = start1 + 64;
  const size_t end2 = end1 + 64;
  for (size_t i = start1; i < end1; ++i)
    CPU_SET(i, &cpuset);
  for (size_t i = start2; i < end2; ++i)
    CPU_SET(i, &cpuset);

  pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
}

template<size_t C>
inline void PoolNgramThread<C>::RunThread()
{
  // Main loop: grab chunks and process them.
  bool done = false;
  size_t fileId = (size_t(-1));
  size_t bytes;
  unsigned char* ptr = NULL;
  do
  {
    // Get the next chunk and the file it corresponds to.
    ptr = NULL;
    bool mustFlush = false;
    while ((ptr == NULL) && (!done))
    {
      mustFlush = reader.GetNextChunk(ptr, bytes, fileId);
      if (ptr == NULL && fileId != size_t(-1))
      {
        // need to wait
        ++waitingForData;
        usleep(1000);
      }
      else if (ptr == NULL && fileId == size_t(-1))
      {
        done = true;
        break;
      }
    }

    // Do we have to flush the file?
    if (mustFlush)
    {
      c.tic();
      threadCounter.flush(globalCounts);
      ++flushCount;
      flushTime += c.toc();
    }

    if (done)
      break;

    if (mustFlush)
      threadCounter.claim();

    // Process the chunk.
    if (bytes >= n)
    {
      c.tic();
      for (size_t i = 0; i < (bytes - (n - 1)); ++i)
        threadCounter.set(ptr + i);
      processTime += c.toc();
    }
  } while (!done);

  // flush the unflushed array if needed
  threadCounter.forceFlush(globalCounts);
}

template<size_t C>
inline void PoolNgramThread<C>::Finish()
{
  if (thread.joinable())
    thread.join();

  if (waitingForData > 0)
  {
    std::ostringstream oss;
    oss << "PoolNgramThread spent " << waitingForData << " iterations waiting"
        << " for a chunk." << std::endl;
    std::cout << oss.str();
  }

  if (threadCounter.waitingForBitset > 0)
  {
    std::ostringstream oss;
    oss << "PoolThreadHashCounter spent " << threadCounter.waitingForBitset
        << " iterations waiting for a bitset." << std::endl;
    std::cout << oss.str();
  }

  std::ostringstream oss;
  oss << "PoolNgramThread: " << processTime << "s processing, " << flushTime
      << "s flushing, " << threadCounter.flushes << " flushes." << std::endl;
  std::cout << oss.str();
}

#endif
