// single_ngram_thread.hpp: thread that actually does the work for n-gramming
// and assumes it is the only thread working with a SingleReaderThread.
#ifndef PNGRAM_SINGLE_NGRAM_THREAD_HPP
#define PNGRAM_SINGLE_NGRAM_THREAD_HPP

#include "multi_thread_hash_counter.hpp"
#include "single_reader_thread.hpp"
#include <thread>
#include <atomic>
#include <armadillo>

class SingleNgramThread
{
 public:
  SingleNgramThread(SingleReaderThread& reader,
                    CountsArray<>& globalCounts,
                    const size_t n,
                    const size_t t,
                    const size_t verbosity = 1);

  inline void RunThread();
  inline void Finish();

 private:
  SingleReaderThread& reader;
  MultiThreadHashCounter threadCounter;
  CountsArray<>& globalCounts;
  const size_t n;
  size_t waitingForData;
  arma::wall_clock c;
  double processTime;
  double flushTime;
  size_t flushCount;
  size_t t;
  size_t verbosity;

  std::thread thread;
};

#include "single_ngram_thread_impl.hpp"

#endif
