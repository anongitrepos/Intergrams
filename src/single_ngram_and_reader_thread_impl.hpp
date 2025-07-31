// single_ngram_and_reader_thread_impl.hpp: implementation of
// SingleNgramAndReaderThread
#ifndef PNGRAM_SINGLE_NGRAM_AND_READER_THREAD_IMPL_HPP
#define PNGRAM_SINGLE_NGRAM_AND_READER_THREAD_IMPL_HPP

#include "single_ngram_thread.hpp"

#include <unistd.h>
#include <iostream>
#include <sys/sysinfo.h>
#include <sched.h>
#include <pthread.h>

inline SingleNgramAndReaderThread::SingleNgramAndReaderThread(
    DirectoryIterator& dIterIn,
    CountsArray<>& globalCounts,
    const size_t n,
    const size_t t,
    const size_t verbosity) :
    dIter(dIterIn),
    globalCounts(globalCounts),
    n(n),
    processTime(0.0),
    flushTime(0.0),
    flushCount(0),
    t(t),
    verbosity(verbosity),
    thread(&SingleNgramAndReaderThread::RunThread, this)
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

inline SingleNgramAndReaderThread::~SingleNgramAndReaderThread()
{
  thread.join();
}

inline void SingleNgramAndReaderThread::RunThread()
{
  // Main loop: grab chunks and process them.
  std::filesystem::path p;
  size_t i;
  while (dIter.get_next(p, i))
  {
    if (i % 10000 == 0 && verbosity > 0)
      std::cout << "Reading file " << i << "..." << std::endl;

    int fd = open(p.c_str(), O_RDONLY); // TODO: O_DIRECT
    if (fd == -1)
    {
      // check errno, something went wrong
      std::cout << "open failed! errno " << errno << "\n";
      continue;
    }

    ssize_t bytesRead;
    do
    {
      bytesRead = read(fd, localBuffer, bufferSize);
      if (bytesRead >= n)
      {
        c.tic();
        for (size_t i = 0; i < (size_t(bytesRead) - (n - 1)); ++i)
          threadCounter.set(localBuffer + i);
        processTime += c.toc();
      }
      else if (bytesRead == -1)
      {
        // error
        std::cout << "read failed! errno " << errno << "\n";
      }

      // rewind for next read, but first check to see if we're at the end of
      // the file
      unsigned char byte;
      size_t testBytes = read(fd, &byte, 1);
      if (testBytes != 0)
      {
        off_t o = -n;
        off_t pos = lseek(fd, o, SEEK_CUR);
        if (lseek(fd, o, SEEK_CUR) == -1)
          std::cout << "lseek fail!\n";
      }
    } while (bytesRead > 0);

    // We are done with the file, so flush.
    c.tic();
    threadCounter.flush(globalCounts);
    ++flushCount;
    flushTime += c.toc();

    close(fd);
  }

  // flush the unflushed array if needed
  threadCounter.forceFlush(globalCounts);
}

#endif
