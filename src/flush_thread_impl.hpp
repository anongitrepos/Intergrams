#ifndef PNGRAM_FLUSH_THREAD_IMPL_HPP
#define PNGRAM_FLUSH_THREAD_IMPL_HPP

#include "flush_thread.hpp"

#include <unistd.h>
#include <iostream>
#include <armadillo>
#include <bit>
#include <sstream>

inline FlushThread::FlushThread(ReaderThread& reader,
                                ThreadHashCounter* thread_counters,
                                std::atomic_uint32_t* gram3_counts) :
    reader(reader),
    thread_counters(thread_counters),
    gram3_counts(gram3_counts),
    flush_time(0.0),
    flushed(0),
    waiting_on_file_to_flush(0),
    thread(&FlushThread::run_thread, this)
{
  // nothing to do
}

inline FlushThread::~FlushThread()
{
  if (thread.joinable())
    thread.join();

  std::ostringstream oss;
  oss << "FlushThread statistics:\n";
  oss << " - Average flush time: " << flush_time / double(flushed) << "s. (" << flushed << " flushes total)\n";
  oss << " - Waited for files " << waiting_on_file_to_flush << " iterations.\n";
  std::cout << oss.str();
}

inline FlushThread::FlushThread(FlushThread&& other) noexcept :
    reader(other.reader),
    thread_counters(other.thread_counters),
    gram3_counts(other.gram3_counts),
    flush_time(other.flush_time),
    flushed(other.flushed),
    waiting_on_file_to_flush(std::move(other.waiting_on_file_to_flush)),
    thread(std::move(other.thread))
{
  // nothing to do
}

inline void FlushThread::run_thread()
{
  size_t indexes[reader.num_files];
  arma::wall_clock c;

  // Periodically check for when we have enough files to flush at once.
  size_t flush_index;
  while (reader.get_next_flushee(flush_index))
  {
    if (flush_index == size_t(-1))
    {
      usleep(1000);
      ++waiting_on_file_to_flush;
      continue;
    }

    c.tic();
    thread_counters[flush_index].flush(gram3_counts);
    flush_time += c.toc();
    flushed++;

    reader.finish_flush(flush_index);
  }
}

#endif
