// compute_ngrams_naive_parallel.cpp:
// Use a bunch of ReaderThreads, and each of them gets a single NgramThread for
// processing.
#include "single_reader_thread.hpp"
#include "single_ngram_thread.hpp"
#include "alloc.hpp"
#include <armadillo>
#include <fstream>

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

  CountsArray globalCounts;
  //std::atomic_uint32_t* globalCounts;
  //alloc_mem_state globalCountsMemState;
  //alloc_hugepage<std::atomic_uint32_t>(globalCounts, globalCountsMemState,
  //    16777216, "n-gramming");
  //memset(globalCounts, 0, sizeof(std::atomic_uint32_t) * 16777216);
  SingleReaderThread** readerThreads = new SingleReaderThread*[threads];
  for (size_t i = 0; i < threads; ++i)
    readerThreads[i] = new SingleReaderThread(iter, 3, i);

  // Allow readers to initialize.
  usleep(1000);

  SingleNgramThread** ngramThreads = new SingleNgramThread*[threads];
  for (size_t i = 0; i < threads; ++i)
  {
    ngramThreads[i] = new SingleNgramThread(*readerThreads[i], globalCounts, 3,
                                            i);
  }

  // Allow ngram threads to initialize.
  usleep(1000);

  for (size_t i = 0; i < threads; ++i)
    ngramThreads[i]->Finish();

  std::cout << "SingleNgramThreads finished (" << c2.toc() << "s)." << std::endl;
  for (size_t i = 0; i < threads; ++i)
  {
    delete readerThreads[i];
    delete ngramThreads[i];
  }

  std::cout << "Total time: " << c.toc() << "s." << std::endl;

  //uint32_t maxCount = 0;
  //for (size_t i = 0; i < 16777216; ++i)
  //  maxCount = std::max(maxCount, (uint32_t) globalCounts[i]);

  //std::cout << "Maximum count: " << maxCount << ".\n";

  std::cout << "Maximum count: " << globalCounts.Maximum() << ".\n";
  //std::fstream f("ngrams-out.csv", std::fstream::out);
  //for (size_t i = 0; i < 16777216; ++i)
  //{
  //  f << "0x" << std::hex << std::setw(6) << std::setfill('0') << i << ", "
  //      << std::dec << (size_t) globalCounts[i] << "\n";
  //}
  //globalCounts.Save("ngrams-out.csv");

  delete[] readerThreads;
  delete[] ngramThreads;
  //free_hugepage<std::atomic_uint32_t>(globalCounts, globalCountsMemState,
  //    16777216);
}
