// group_reader_thread_impl.hpp: implementation of GroupReaderThread functionality.
#ifndef PNGRAM_GROUP_READER_THREAD_IMPL_HPP
#define PNGRAM_GROUP_READER_THREAD_IMPL_HPP

#include "group_reader_thread.hpp"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sched.h>
#include <sys/sysinfo.h>
#include <pthread.h>

template<size_t NumBitsets>
inline GroupReaderThread<NumBitsets>::GroupReaderThread(
    DirectoryIterator& dIterIn,
    GroupThreadHashCounter<NumBitsets>& counterIn,
    const size_t n,
    const size_t t) :
    dIter(dIterIn),
    counter(counterIn),
    localBuffer(new unsigned char[totalBufferSize]),
    chunkSizes(new size_t[numChunks]),
    nextAvailableChunk(0),
    nextUnusedChunk(0),
    n(n),
    waitingForFiles(0),
    waitingForChunks(0),
    currentFile(size_t(-1)),
    thread(&GroupReaderThread::RunThread, this),
    finished(false)
{
  for (size_t i = 0; i < numFiles; ++i)
    fileIds[i] = size_t(-1);

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
inline GroupReaderThread<NumBitsets>::~GroupReaderThread()
{
  thread.join();

  bool done = false;
  while (!done)
  {
    bool anyLeft = false;
    for (size_t i = 0; i < numChunks; ++i)
    {
      if (chunkStatus[i] != 0)
      {
        std::cout << "Chunk " << i << " status is " << chunkStatus[i] << "!\n";
        anyLeft = true;
        break;
      }
    }

    if (!anyLeft)
    {
      for (size_t i = 0; i < numFiles; ++i)
      {
        if (fileChunksLeft[i] > 0)
        {
          std::cout << "File " << i << " has " << fileChunksLeft[i] << " chunks left!\n";
          anyLeft = true;
          break;
        }
      }
    }

    if (!anyLeft)
    {
      done = true;
      break;
    }

    usleep(1000000);
  }

  std::ostringstream oss;
  oss << "GroupReaderThread statistics:\n";
  oss << " - " << waitingForFiles << " iterations waiting on a file to be open.\n";
  oss << " - " << waitingForChunks << " iterations waiting on a chunk to be available.\n";
  std::cout << oss.str();

  delete[] localBuffer;
  delete[] chunkSizes;
}

template<size_t NumBitsets>
inline void GroupReaderThread<NumBitsets>::RunThread()
{
  // mark all chunks unclaimed and unavailable
  for (size_t i = 0; i < numChunks; ++i)
  {
    chunkStatus[i] = 0;
    chunkFileIds[i] = 0;
  }

  for (size_t i = 0; i < numFiles; ++i)
    fileChunksLeft[i] = 0;

  std::filesystem::path p;
  size_t i;
  while (dIter.get_next(p, i))
  {
    if (i % 10000 == 0)
      std::cout << "Reading file " << i << "..." << std::endl;

    bool foundFileIndex = false;
    while (!foundFileIndex)
    {
      for (size_t index = 0; index < numFiles; ++index)
      {
        if (fileChunksLeft[index] == 0)
        {
          currentFile = index;
          fileIds[currentFile] = i;
          foundFileIndex = true;
          break;
        }
      }

      // Maybe chunks have not been processed... wait for a little while.
      if (!foundFileIndex)
      {
        ++waitingForFiles;
        usleep(1000);
      }
    }

    // extra count of 1 indicates we are still reading
    fileChunksLeft[currentFile] = 1;

    int fd = open(p.c_str(), O_RDONLY); // TODO: O_DIRECT
    if (fd == -1)
    {
      // check errno, something went wrong
      std::cout << "open failed! errno " << errno << "\n";
      fileChunksLeft[currentFile] = 0;
      fileIds[currentFile] = size_t(-1);
      continue;
    }

    // read into buffer
    ssize_t bytesRead = 0;
    do
    {
      // Find the chunk we will read into.  There are cases in which we will
      // need to wait...
      size_t targetChunk = nextUnusedChunk;
      bool found = false;
      while (true)
      {
        do
        {
          uint8_t expected = 0;
          if (chunkStatus[targetChunk].compare_exchange_strong(expected, 1))
          {
            // We got and claimed our target chunk.
            found = true;
            break;
          }
          targetChunk = (targetChunk + 1) % numChunks;
        } while (targetChunk != nextUnusedChunk);

        // Did we find a chunk at all?
        if (found)
          break;

        // We didn't find a chunk, so we need to wait...
        ++waitingForChunks;
        usleep(1000);
      }

      bytesRead = read(fd, localBuffer + targetChunk * chunkSize, chunkSize);
      if (bytesRead > 0)
      {
        // advance to next chunk
        ++fileChunksLeft[currentFile];
        chunkFileIds[targetChunk] = currentFile;
        chunkSizes[targetChunk] = bytesRead;
        chunkStatus[targetChunk] = 2; // ready for processing
        nextAvailableChunk = targetChunk;

        // rewind for next read
        off_t o = -(n - 1);
        if (lseek(fd, -o, SEEK_CUR) == -1)
          std::cout << "lseek fail!\n";
      }
      else if (bytesRead == -1)
      {
        // error
        chunkStatus[targetChunk] = 0;
        std::cout << "read failed! errno " << errno << "\n";
      }
      else
      {
        // no bytes read, relinquish the chunk
        chunkStatus[targetChunk] = 0;
      }
    } while (bytesRead > 0);

    // since we are done with the file, remove the extra chunk count
    // if we are done with the file, mark it as complete
    if (--fileChunksLeft[currentFile] == 0)
    {
      // We have to mark the bitset that goes with this file as finished.
      size_t bitsetId = counter.GetExistingBitsetId(i);
      counter.bitsetStatuses[bitsetId] = 2;
      counter.fileIds[bitsetId] = size_t(-1);
      fileIds[currentFile] = size_t(-1);
    }

    close(fd);
  }

  finished = true;
}

template<size_t NumBitsets>
inline bool GroupReaderThread<NumBitsets>::GetNextChunk(unsigned char*& ptr,
                                                        size_t& bytes,
                                                        size_t& fileId,
                                                        size_t& readerFileId)
{
  // When we search for a chunk, ideally we want to try and find a chunk for the
  // same file if possible.  If fileId == -1, though, then we don't have an
  // assigned file, so just take whatever.
  size_t targetChunk = nextAvailableChunk;
  bool found = false;

  if (fileId != size_t(-1))
  {
    // Check to see if there are any chunks available for our desired file.
    do
    {
      uint8_t expected = 2;
      if (chunkFileIds[targetChunk] == readerFileId)
      {
        if (chunkStatus[targetChunk].compare_exchange_strong(expected, 3))
        {
          found = true;
          break;
        }
      }

      targetChunk = (targetChunk + 1) % numChunks;
    } while (targetChunk != nextAvailableChunk);
  }

  // If we didn't find anything for our file ID (or if we didn't search), now
  // find a chunk for the 'oldest' file ID that has any chunks.
  if (!found)
  {
    for (size_t targetFileId = currentFile + numFiles - 1;
         targetFileId + 1 >= currentFile + 1 /* avoid underflow */;
         --targetFileId)
    {
      if (fileChunksLeft[targetFileId % numFiles] > 0)
      {
        readerFileId = targetFileId % numFiles;
        fileId = fileIds[readerFileId];
        break;
      }
    }

    do
    {
      uint8_t expected = 2;
      if (chunkFileIds[targetChunk] == readerFileId)
      {
        if (chunkStatus[targetChunk].compare_exchange_strong(expected, 3))
        {
          found = true;
          break;
        }
      }

      targetChunk = (targetChunk + 1) % numChunks;
    } while (targetChunk != nextAvailableChunk);
  }

  if (found)
  {
    // We got a chunk, so, assign it.  The status is already set.
    ptr = localBuffer + targetChunk * chunkSize;
    bytes = chunkSizes[targetChunk];
    // Note that it is possible this might not be the original file ID!
    readerFileId = chunkFileIds[targetChunk];
    fileId = fileIds[readerFileId];
    return false;
  }
  else
  {
    // We didn't find a chunk.
    ptr = NULL;
    bytes = 0;
    return finished;
  }
}

template<size_t NumBitsets>
inline bool GroupReaderThread<NumBitsets>::FinishChunk(const unsigned char* ptr,
                                                       const size_t readerFileId)
{
  const size_t chunkId = (ptr - localBuffer) / chunkSize;
  const size_t newChunksLeft = --fileChunksLeft[readerFileId];
  chunkStatus[chunkId] = 0; // We are done with the chunk.
  if (newChunksLeft == 0)
    fileIds[readerFileId] = size_t(-1);

  nextUnusedChunk = chunkId;

  // If no chunks are left, then the file needs to be flushed.
  return (newChunksLeft == 0);
}

#endif
