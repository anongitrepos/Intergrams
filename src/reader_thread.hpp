// reader_thread.hpp: Definition of ReaderThread, which reads through files and
// puts bytes into a buffer.
#ifndef PNGRAM_READER_THREAD_HPP
#define PNGRAM_READER_THREAD_HPP

#include "thread_hash_counter.hpp"
#include "directory_iterator.hpp"
#include <thread>
#include <atomic>

class ReaderThread
{
 public:
  inline ReaderThread(DirectoryIterator& directory, const size_t n);
  inline ~ReaderThread();

  inline void thread_run();

  // returns NULL if there is no chunk right now for ptr, and returns false if
  // reading is done and there are no more chunks
  inline bool get_next_chunk(unsigned char*& ptr, size_t& bytes, size_t& file_id);

  // returns true if this is the last outstanding chunk for this file
  inline void finish_chunk(const unsigned char* ptr, const size_t file_id);

  inline bool get_next_flushee(size_t& file_id);
  inline void finish_flush(const size_t file_id);

  // read 4KB at a time
  static constexpr size_t chunk_size = 4096;
  static constexpr size_t total_buffer_size = (1 << 18); // buffer up to 256KB of data
  static constexpr size_t num_chunks = total_buffer_size / chunk_size;
  static constexpr size_t num_files = 8;

 private:
  DirectoryIterator& d_it;
  unsigned char* local_buffer;
  size_t* chunk_sizes;
  size_t next_available_chunk;
  size_t next_unused_chunk;
  size_t n;
  size_t waiting_for_files;
  size_t waiting_for_chunks;

  // true indicates that the chunk has valid unprocessed data
  std::atomic_bool chunk_ready[num_chunks];
  // true indicates that the chunk is currently being processed and should not
  // be modified
  std::atomic_bool chunk_claimed[num_chunks];
  uint8_t chunk_file_ids[num_chunks];

  std::atomic_uint32_t file_chunks_left[num_files];
  std::atomic_uint32_t needs_flush_count;

  std::thread thread;
  std::mutex next_chunk_mutex;

  bool finished;
};

#include "reader_thread_impl.hpp"

#endif
