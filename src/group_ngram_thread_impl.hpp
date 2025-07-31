// group_ngram_thread_impl.hpp: implementation of SingleNgramThread
#ifndef PNGRAM_GROUP_NGRAM_THREAD_IMPL_HPP
#define PNGRAM_GROUP_NGRAM_THREAD_IMPL_HPP

#include "group_ngram_thread.hpp"

#include <unistd.h>
#include <iostream>
#include <sys/sysinfo.h>
#include <sched.h>
#include <pthread.h>

template<size_t NumBitsets>
inline GroupNgramThread<NumBitsets>::GroupNgramThread(
    GroupReaderThread<NumBitsets>& reader,
    GroupThreadHashCounter<NumBitsets>& counter,
    const size_t n,
    const size_t t) :
    reader(reader),
    counter(counter),
    n(n),
    waitingForData(0),
    processTime(0.0),
    flushTime(0.0),
    flushCount(0),
    t(t),
    thread(&GroupNgramThread<NumBitsets>::RunThread, this)
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

template<size_t NumBitsets>
inline GroupNgramThread<NumBitsets>::~GroupNgramThread()
{
  // Try to flush anything until we can't.
  counter.ForceFlush();
}

template<size_t NumBitsets>
inline void GroupNgramThread<NumBitsets>::RunThread()
{
  // Main loop: grab chunks and process them.
  bool done = false;
  size_t lastFileId = size_t(-2);
  size_t fileId = size_t(-1);
  size_t readerFileId = size_t(-1);
  size_t bytes;
  unsigned char* ptr = NULL;
  size_t bitsetId = size_t(-1);
  do
  {
    // Get the next chunk and the file it corresponds to.
    ptr = NULL;
    while ((ptr == NULL) && (!done))
    {
      done = reader.GetNextChunk(ptr, bytes, fileId, readerFileId);
      if (!done && ptr == NULL)
      {
        // need to wait
        ++waitingForData;
        usleep(1000);
      }
    }

    if (ptr != NULL)
    {
      // Get the bitset ID for this file.
      if (fileId != lastFileId)
      {
        lastFileId = fileId;
        bitsetId = counter.GetBitsetId(fileId);
      }

      // Process the chunk.
      if (bytes >= n)
      {
        c.tic();
        for (size_t i = 0; i < (bytes - (n - 1)); ++i)
          counter.Set(ptr + i, bitsetId);
        processTime += c.toc();
      }

      // Indicate the chunk is finished.
      const bool mustFlush = reader.FinishChunk(ptr, readerFileId);

      // Do we have to flush the file?
      if (mustFlush)
      {
        ++flushCount;
        c.tic();
        counter.FinishBitset(bitsetId, done);
        lastFileId = size_t(-1); // We are definitely changing files next time.
        bitsetId = size_t(-1);
        flushTime += c.toc();
      }
    }
  } while (!done);
}

template<size_t NumBitsets>
inline void GroupNgramThread<NumBitsets>::Finish()
{
  if (thread.joinable())
    thread.join();

  if (waitingForData > 0)
  {
    std::ostringstream oss;
    oss << "GroupNgramThread spent " << waitingForData << " iterations waiting"
        << " for a chunk." << std::endl;
    std::cout << oss.str();
  }

  std::ostringstream oss;
  oss << "GroupNgramThread: " << processTime << "s processing, " << flushTime
      << "s flushing, " << flushCount << " flushes." << std::endl;
  std::cout << oss.str();
}

#endif
