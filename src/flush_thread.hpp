#ifndef PNGRAM_FLUSH_THREAD_HPP
#define PNGRAM_FLUSH_THREAD_HPP

#include "reader_thread.hpp"
#include "thread_hash_counter.hpp"
#include <atomic>
#include <thread>

class FlushThread
{
 public:
  FlushThread(ReaderThread& reader,
              ThreadHashCounter* thread_counters,
              std::atomic_uint32_t* gram3_counts);
  ~FlushThread();

  FlushThread(FlushThread&& other) noexcept;

  inline void run_thread();

 private:
  ReaderThread& reader;
  ThreadHashCounter* thread_counters;
  std::atomic_uint32_t* gram3_counts;
  double flush_time;
  size_t flushed;
  size_t waiting_on_file_to_flush;

  std::thread thread;
};

#include "flush_thread_impl.hpp"

#endif
