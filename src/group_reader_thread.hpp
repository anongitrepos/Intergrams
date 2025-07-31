// group_reader_thread.hpp: Basically the same as ReaderThread, but returns the
// actual file IDs as given by the DirectoryIterator.
#ifndef PNGRAM_GROUP_READER_THREAD_HPP
#define PNGRAM_GROUP_READER_THREAD_HPP

#include "directory_iterator.hpp"
#include "group_thread_hash_counter.hpp"
#include <thread>
#include <atomic>

template<size_t NumBitsets>
class GroupReaderThread
{
 public:
  inline GroupReaderThread(DirectoryIterator& directory,
                           GroupThreadHashCounter<NumBitsets>& counter,
                           const size_t n,
                           const size_t t);
  inline ~GroupReaderThread();

  inline void RunThread();

  // returns NULL if there is no chunk right now for ptr, and returns false if
  // reading is done and there are no more chunks
  inline bool GetNextChunk(unsigned char*& ptr, size_t& bytes, size_t& fileId,
                           size_t& readerFileId);

  // returns true if this is the last outstanding chunk for this file
  inline bool FinishChunk(const unsigned char* ptr, const size_t readerFileId);

  // read 4KB at a time
  static constexpr size_t chunkSize = 4096;
  static constexpr size_t totalBufferSize = (1 << 20); // buffer up to 1MB of data
  static constexpr size_t numChunks = totalBufferSize / chunkSize;
  static constexpr size_t numFiles = 4;

 private:
  DirectoryIterator& dIter;
  GroupThreadHashCounter<NumBitsets>& counter;
  unsigned char* localBuffer;
  size_t* chunkSizes;
  size_t nextAvailableChunk;
  size_t nextUnusedChunk;
  size_t n;
  size_t waitingForFiles;
  size_t waitingForChunks;
  size_t currentFile;

  // 0: available
  // 1: reading (not ready)
  // 2: ready for processing
  // 3: claimed
  std::atomic_uint8_t chunkStatus[numChunks];
  uint8_t chunkFileIds[numChunks];
  size_t fileIds[numFiles];

  std::atomic_uint32_t fileChunksLeft[numFiles];

  std::thread thread;
  std::mutex nextChunkMutex;

  bool finished;
};

#include "group_reader_thread_impl.hpp"

#endif
