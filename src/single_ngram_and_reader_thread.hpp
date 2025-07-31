// single_ngram_and_reader_thread.hpp: thread that actually does the work for
// n-gramming, including reading the file.
#ifndef PNGRAM_SINGLE_NGRAM_AND_READER_THREAD_HPP
#define PNGRAM_SINGLE_NGRAM_AND_READER_THREAD_HPP

#include "multi_thread_hash_counter.hpp"
#include <thread>
#include <atomic>
#include <armadillo>

class SingleNgramAndReaderThread
{
 public:
  SingleNgramAndReaderThread(DirectoryIterator& dIterIn,
                             CountsArray<>& globalCounts,
                             const size_t n,
                             const size_t t,
                             const size_t verbosity = 1);

  ~SingleNgramAndReaderThread();

  inline void RunThread();

  const static size_t bufferSize = 4096;

 private:
  DirectoryIterator& dIter;
  unsigned char localBuffer[bufferSize];
  MultiThreadHashCounter threadCounter;
  CountsArray<>& globalCounts;
  const size_t n;
  arma::wall_clock c;
  double processTime;
  double flushTime;
  size_t flushCount;
  size_t t;
  size_t verbosity;

  std::thread thread;
};

#include "single_ngram_and_reader_thread_impl.hpp"

#endif
