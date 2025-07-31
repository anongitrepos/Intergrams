// single_reader_thread.hpp: Definition of SingleReaderThread, which reads
// through files and puts bytes into a buffer.  This expects that only one
// thread is reading at a time.
#ifndef PNGRAM_SINGLE_READER_THREAD_HPP
#define PNGRAM_SINGLE_READER_THREAD_HPP

#include "directory_iterator.hpp"
#include <thread>
#include <atomic>

class SingleReaderThread
{
 public:
  inline SingleReaderThread(DirectoryIterator& directory,
                            const size_t n,
                            const size_t t,
                            const size_t verbosity = 1);
  inline ~SingleReaderThread();

  inline void RunThread();

  // returns NULL if there is no chunk right now for ptr, and returns false if
  // reading is done and there are no more chunks
  inline bool GetNextChunk(unsigned char*& ptr, size_t& bytes, size_t& file_id);

  // read 4KB at a time
  static constexpr size_t chunkSize = 4096;
  static constexpr size_t totalBufferSize = (1 << 18); // buffer up to 256KB of data
  static constexpr size_t numChunks = totalBufferSize / chunkSize;

 private:
  DirectoryIterator& dIter;
  unsigned char* localBuffer;
  size_t* chunkSizes;
  size_t* chunkFileIds;
  std::atomic<size_t> readChunkId;
  std::atomic<size_t> processChunkId;
  size_t n;
  size_t waitingForChunks;
  size_t verbosity;

  std::thread thread;

  bool finished;
};

#include "single_reader_thread_impl.hpp"

#endif
