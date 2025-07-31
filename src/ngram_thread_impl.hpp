// ngram_thread_impl.hpp: implementation of NgramThread
#ifndef PNGRAM_NGRAM_THREAD_IMPL_HPP
#define PNGRAM_NGRAM_THREAD_IMPL_HPP

#include "ngram_thread.hpp"

#include <unistd.h>
#include <iostream>

inline NgramThread::NgramThread(ReaderThread& reader,
                                ThreadHashCounter* thread_counters,
                                size_t n) :
  reader(reader),
  thread_counters(thread_counters),
  n(n),
  waiting_on_available_chunk(0),
  thread(&NgramThread::run_thread, this)
{
  // Nothing to do.
}

inline NgramThread::~NgramThread()
{
  if (thread.joinable())
    thread.join();

  if (waiting_on_available_chunk > 0)
  {
    std::ostringstream oss;
    oss << "NgramThread spent " << waiting_on_available_chunk
        << " iterations waiting for a chunk." << std::endl;
    std::cout << oss.str();
  }
}

inline NgramThread::NgramThread(NgramThread&& other) noexcept :
    reader(other.reader),
    thread_counters(other.thread_counters),
    n(other.n),
    waiting_on_available_chunk(std::move(other.waiting_on_available_chunk)),
    thread(std::move(other.thread))
{
  // Nothing to do.
}

inline void NgramThread::run_thread()
{
  // Main loop: grab chunks and process them.
  bool done = false;
  do
  {
    // Get the next chunk and the file it corresponds to.
    size_t file_id = size_t(-1);
    size_t bytes;
    unsigned char* ptr = NULL;
    while ((ptr == NULL) && (!done))
    {
      done = !reader.get_next_chunk(ptr, bytes, file_id);
      if (ptr == NULL)
      {
        // need to wait
        ++waiting_on_available_chunk;
        usleep(1000);
      }
    }

    // Process the chunk.
    if (bytes >= n)
    {
      uint64_t mask = (1 << file_id);

      for (size_t i = 0; i < (bytes - n); ++i)
        thread_counters[file_id].set(ptr + i);

      reader.finish_chunk(ptr, file_id);
    }
    else if (ptr != NULL)
    {
      reader.finish_chunk(ptr, file_id);
    }
  } while (!done);
}

#endif
