// ngram_thread.hpp: thread that actually does the work for n-gramming
#ifndef PNGRAM_NGRAM_THREAD_HPP
#define PNGRAM_NGRAM_THREAD_HPP

#include "thread_hash_counter.hpp"
#include "reader_thread.hpp"
#include <thread>
#include <atomic>

class NgramThread
{
 public:
  NgramThread(ReaderThread& reader, ThreadHashCounter* thread_counters, const size_t n);
  ~NgramThread();

  NgramThread(NgramThread&& other) noexcept;

  inline void run_thread();

 private:
  ReaderThread& reader;
  ThreadHashCounter* thread_counters;
  const size_t n;
  size_t waiting_on_available_chunk;

  std::thread thread;
};

#include "ngram_thread_impl.hpp"

#endif
