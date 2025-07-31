// pool_ngram_thread.hpp: thread that actually does the work for n-gramming.  It
// shares bit vectors with other SingleNgramThreads on the same physical
// processor.
#ifndef PNGRAM_POOL_NGRAM_THREAD_HPP
#define PNGRAM_POOL_NGRAM_THREAD_HPP

#include "pool_thread_hash_counter.hpp"
#include "single_reader_thread.hpp"
#include <thread>
#include <atomic>
#include <armadillo>

template<size_t C>
class PoolNgramThread
{
 public:
  PoolNgramThread(SingleReaderThread& reader,
                  CountsArray& globalCounts,
                  uint64_t** bitsets,
                  std::atomic_uint8_t* bitsetStatuses,
                  const size_t n,
                  const size_t t);

  inline void RunThread();
  inline void Finish();

 private:
  SingleReaderThread& reader;
  PoolThreadHashCounter<C> threadCounter;
  CountsArray& globalCounts;
  const size_t n;
  size_t waitingForData;
  arma::wall_clock c;
  double processTime;
  double flushTime;
  size_t flushCount;
  size_t t;

  std::thread thread;
};

#include "pool_ngram_thread_impl.hpp"

#endif
