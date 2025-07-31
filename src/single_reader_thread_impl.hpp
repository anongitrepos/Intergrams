// single_reader_thread_impl.hpp: implementation of SingleReaderThread functionality.
#ifndef PNGRAM_SINGLE_READER_THREAD_IMPL_HPP
#define PNGRAM_SINGLE_READER_THREAD_IMPL_HPP

#include "reader_thread.hpp"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sched.h>
#include <sys/sysinfo.h>
#include <pthread.h>

inline SingleReaderThread::SingleReaderThread(DirectoryIterator& iterIn,
                                              const size_t n,
                                              const size_t t,
                                              const size_t verbosity) :
    dIter(iterIn),
    localBuffer(new unsigned char[totalBufferSize]),
    chunkSizes(new size_t[numChunks]),
    chunkFileIds(new size_t[numChunks]),
    readChunkId(0),
    processChunkId(numChunks - 1),
    n(n),
    waitingForChunks(0),
    thread(&SingleReaderThread::RunThread, this),
    finished(false),
    verbosity(verbosity)
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

inline SingleReaderThread::~SingleReaderThread()
{
  thread.join();

  bool done = false;
  while (processChunkId < readChunkId)
    usleep(1000);

  if (verbosity > 1)
  {
    std::ostringstream oss;
    oss << "SingleReaderThread: " << waitingForChunks
        << " iterations waiting on a chunk to be available." << std::endl;
    std::cout << oss.str();
  }

  delete[] localBuffer;
  delete[] chunkSizes;
  delete[] chunkFileIds;
}

inline void SingleReaderThread::RunThread()
{
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

    // read into buffer
    ssize_t bytesRead = 0;
    do
    {
      // Wait, if needed.
      while (((readChunkId + 1) % numChunks) == processChunkId)
      {
        usleep(1000);
        ++waitingForChunks;
      }

      bytesRead = read(fd, localBuffer + readChunkId * chunkSize, chunkSize);
      if (bytesRead > 0)
      {
        // advance to next chunk
        chunkSizes[readChunkId] = bytesRead;
        chunkFileIds[readChunkId] = i;

        readChunkId = ((readChunkId + 1) % numChunks);

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
      }
      else if (bytesRead == -1)
      {
        // error
        std::cout << "read failed! errno " << errno << "\n";
      }
    } while (bytesRead > 0);

    close(fd);
  }

  finished = true;
}

inline bool SingleReaderThread::GetNextChunk(unsigned char*& ptr,
                                             size_t& bytes,
                                             size_t& fileId)
{
  if (finished)
  {
    if (processChunkId != readChunkId)
    {
      ++processChunkId;
      if (processChunkId >= numChunks)
        processChunkId = 0;

      if (processChunkId == readChunkId)
      {
        ptr = nullptr;
        bytes = 0;
        fileId = size_t(-1);
        // always flush when we're done
        return true;
      }

      ptr = localBuffer + processChunkId * chunkSize;
      bytes = chunkSizes[processChunkId];
      const size_t oldFileId = fileId;
      fileId = chunkFileIds[processChunkId];

      return ((fileId != oldFileId) && (oldFileId != size_t(-1)));
    }
    else
    {
      ptr = nullptr;
      bytes = 0;
      fileId = size_t(-1);
      // always flush when we're done
      return true;
    }
  }
  else
  {
    // Reading is not finished yet.
    if (((processChunkId + 1) % numChunks) == readChunkId)
    {
      // Wait for chunk to be available.
      ptr = nullptr;
      bytes = 0;
      return false;
    }
    else
    {
      processChunkId = ((processChunkId + 1) % numChunks);

      ptr = localBuffer + processChunkId * chunkSize;
      bytes = chunkSizes[processChunkId];
      const size_t oldFileId = fileId;
      fileId = chunkFileIds[processChunkId];

      return ((fileId != oldFileId) && (oldFileId != size_t(-1)));
    }
  }
}

#endif
