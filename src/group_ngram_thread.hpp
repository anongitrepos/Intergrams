// pool_ngram_thread.hpp: thread that actually does the work for n-gramming
// and assumes it is the only thread working with a SingleReaderThread.  It
// shares bit vectors with other SingleNgramThreads on the same physical
// processor.
#ifndef PNGRAM_GROUP_NGRAM_THREAD_HPP
#define PNGRAM_GROUP_NGRAM_THREAD_HPP

#include "group_reader_thread.hpp"
#include "group_thread_hash_counter.hpp"
#include <thread>
#include <atomic>
#include <armadillo>

// These work in groups of threads with a single reader.
template<size_t NumBitsets>
class GroupNgramThread
{
 public:
  GroupNgramThread(GroupReaderThread<NumBitsets>& reader,
                   GroupThreadHashCounter<NumBitsets>& counter,
                   const size_t n,
                   const size_t t);

  ~GroupNgramThread();

  inline void RunThread();
  inline void Finish();

 private:
  GroupReaderThread<NumBitsets>& reader;
  GroupThreadHashCounter<NumBitsets>& counter;
  const size_t n;
  size_t waitingForData;
  arma::wall_clock c;
  double processTime;
  double flushTime;
  size_t flushCount;
  size_t t;

  std::thread thread;
  size_t groupThreadId;
};

#include "group_ngram_thread_impl.hpp"

#endif
