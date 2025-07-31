// lockstep_reader_thread.hpp: Definition of LockstepReaderThread, which reads
// through files puts bytes into a buffer.  Threads reading from this are
// expected to proceed in lockstep... or something like that.
#ifndef PNGRAM_LOCKSTEP_READER_THREAD_HPP
#define PNGRAM_LOCKSTEP_READER_THREAD_HPP

#include "directory_iterator.hpp"
#include <thread>
#include <atomic>
#include "alloc.hpp"
#include "simd_util.hpp"

class LockstepReaderThread
{
 public:
  inline LockstepReaderThread(std::filesystem::path directory, const size_t n);
  inline ~LockstepReaderThread();

  inline void RunThread();

  // Returns false if we're done with all chunks.
  inline bool GetNextChunk(u512*& ptr,
                           size_t& bytes,
                           size_t& fileId,
                           size_t& chunkId,
                           const size_t threadId);

  inline void FinishChunk(const size_t chunkId, const size_t threadId);

  // read 16KB at a time
  static constexpr size_t chunkSize = 4096;
  static constexpr size_t chunkStep = chunkSize / 64;
  static constexpr size_t totalBufferSize = (1 << 23); // buffer up to 16MB of data
  static constexpr size_t numChunks = totalBufferSize / chunkSize;

 private:
  DirectoryIterator dirIter;
  u512* localBuffer;
  alloc_mem_state localBufferMemState;
  size_t chunkSizes[numChunks];
  size_t chunkFileIds[numChunks];
  size_t nextChunk;
  size_t n;
  size_t waitingForChunks;

  // Indicates how many threads must still report completion for a chunk.
  std::atomic_uint8_t chunkDoneCount[numChunks];

  std::thread thread;

  bool finished;
};

#include "lockstep_reader_thread_impl.hpp"

#endif
