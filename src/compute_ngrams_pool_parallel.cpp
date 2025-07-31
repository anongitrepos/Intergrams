// compute_ngrams_naive_parallel.cpp:
// Use a bunch of ReaderThreads, and each of them gets a single NgramThread for
// processing.
#include "single_reader_thread.hpp"
#include "pool_ngram_thread.hpp"
#include "alloc.hpp"
#include <armadillo>
#include <fstream>

constexpr const size_t C = 128;
constexpr const size_t P = 4;

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " directory/ <n_threads>" << std::endl;
    exit(1);
  }

  std::filesystem::path directory(argv[1]);
  size_t threads = atoi(argv[2]);
  DirectoryIterator iter({ directory }, false);

  arma::wall_clock c, c2;
  c.tic();
  c2.tic();

  CountsArray globalCounts[4];
  SingleReaderThread** readerThreads = new SingleReaderThread*[threads];
  for (size_t i = 0; i < threads; ++i)
    readerThreads[i] = new SingleReaderThread(iter, 3, i);

  // Allow readers to initialize.
  usleep(100);

  // Create all of the bitsets that we will use.
  alignas(64) uint64_t* bitsets;
  alloc_mem_state bitsetsMemState;
  alloc_hugepage<uint64_t>(bitsets, bitsetsMemState, P * C * 262144,
      "n-gramming");
  memset(bitsets, 0, sizeof(uint64_t) * P * C * 262144);

  std::atomic_uint8_t* bitsetStatuses = new std::atomic_uint8_t[P * C];
  memset(bitsetStatuses, 0, sizeof(std::atomic_uint8_t) * P * C);

  uint64_t* bitsetPtrs[P][C];
  std::atomic_uint8_t* bitsetStatusPtrs[P];
  for (size_t p = 0; p < P; ++p)
  {
    for (size_t c = 0; c < C; ++c)
      bitsetPtrs[p][c] = bitsets + 262144 * (p * C + c);

    bitsetStatusPtrs[p] = bitsetStatuses + p * C;
  }

  PoolNgramThread<C>** ngramThreads = new PoolNgramThread<C>*[threads];
  for (size_t i = 0; i < threads; ++i)
  {
    ngramThreads[i] = new PoolNgramThread<C>(*readerThreads[i],
        globalCounts[i % 4], bitsetPtrs[i % 4], bitsetStatusPtrs[i % 4], 3, i);
  }

  // Allow ngram threads to initialize.
  usleep(1000);

  for (size_t i = 0; i < threads; ++i)
    ngramThreads[i]->Finish();

  std::cout << "PoolNgramThreads finished (" << c2.toc() << "s)." << std::endl;
  for (size_t i = 0; i < threads; ++i)
  {
    delete readerThreads[i];
    delete ngramThreads[i];
  }

  std::cout << "Total time: " << c.toc() << "s." << std::endl;

  std::cout << "Maximum count: " << globalCounts[0].Maximum() +
globalCounts[1].Maximum() + globalCounts[2].Maximum() +
globalCounts[3].Maximum() << ".\n";
  //std::fstream f("ngrams-out.csv", std::fstream::out);
  //for (size_t i = 0; i < 16777216; ++i)
  //{
  //  f << "0x" << std::hex << std::setw(6) << std::setfill('0') << i << ", "
  //      << std::dec << (size_t) globalCounts[i] << "\n";
  //}
  //globalCounts.Save("ngrams-out.csv");

  delete[] readerThreads;
  delete[] ngramThreads;
  free_hugepage<uint64_t>(bitsets, bitsetsMemState, P * C * 262144);
  delete[] bitsetStatuses;
}
