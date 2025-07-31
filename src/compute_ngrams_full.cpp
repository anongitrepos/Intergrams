// compute_ngrams_full.cpp:
// Use a bunch of ReaderThreads, and each of them gets a single NgramThread for
// processing.  This supports n > 3.
#include "single_reader_thread.hpp"
#include "single_ngram_thread.hpp"
#include "prefix_single_ngram_thread.hpp"
#include "packed_byte_trie.hpp"
#include "find_top_k.hpp"
#include "alloc.hpp"
#include <armadillo>
#include <fstream>

int main(int argc, char** argv)
{
  if (argc != 9)
  {
    std::cerr << "Usage: " << argv[0] << " directory/ <n> <k> <overage> "
        << "<n_threads> <verbosity> <save_intermediate> <output_file_prefix>" << std::endl;
    std::cout << " - if save_intermediate is 1, then you get e.g. <output_file_prefix>.3.txt, etc." << std::endl;
    std::cout << " - try <output_file_prefix> as just 'ngrams' to get 'ngrams.n.txt'" << std::endl;
    exit(1);
  }

  std::filesystem::path directory(argv[1]);
  size_t n = atoi(argv[2]);
  size_t k = atoi(argv[3]);
  double overage = atof(argv[4]);
  size_t threads = atoi(argv[5]);
  size_t verbosity = atoi(argv[6]);
  size_t saveIntermediate = atoi(argv[7]);
  std::string outputPrefix(argv[8]);

  //
  // First pass: 3-grams.
  //

  DirectoryIterator iter({ directory }, false);

  arma::wall_clock overallC, stepC;
  overallC.tic();
  stepC.tic();

  CountsArray globalCounts;
  SingleReaderThread** readerThreads = new SingleReaderThread*[threads];
  for (size_t i = 0; i < threads; ++i)
    readerThreads[i] = new SingleReaderThread(iter, 3, i, verbosity);

  // Allow readers to initialize.
  usleep(500);

  SingleNgramThread** ngramThreads = new SingleNgramThread*[threads];
  for (size_t i = 0; i < threads; ++i)
  {
    ngramThreads[i] = new SingleNgramThread(*readerThreads[i], globalCounts, 3,
        i, verbosity);
  }

  // Allow ngram threads to initialize.
  usleep(1000);

  for (size_t i = 0; i < threads; ++i)
    ngramThreads[i]->Finish();

  for (size_t i = 0; i < threads; ++i)
  {
    delete readerThreads[i];
    delete ngramThreads[i];
  }

  std::cout << "3-gram computation time: " << stepC.toc() << "s." << std::endl;

  delete[] readerThreads;
  delete[] ngramThreads;

  // Now sort the top-k.
  stepC.tic();
  size_t keepSize = size_t(double(k) * (n == 3 ? 1.0 : overage));
  uint8_t* prefixes;
  alloc_mem_state prefixMemState;
  alloc_hugepage<uint8_t>(prefixes, prefixMemState, 3 * keepSize,
      "top-k computation");
  uint32_t* prefixCounts = new uint32_t[keepSize];
  keepSize = FindTopK(globalCounts, 3, keepSize, prefixes, prefixCounts);
  std::cout << "Top-k computation time: " << stepC.toc() << "s (actually got "
      << keepSize << " prefixes)." << std::endl;

  // Write to file if needed.
  if (saveIntermediate == 1 || n == 3)
  {
    stepC.tic();
    arma::Col<uint32_t> countsVec(prefixCounts, keepSize, false, true);
    arma::uvec countOrder = arma::sort_index(countsVec, "descend");

    std::fstream of(outputPrefix + ".3.txt", std::fstream::out);
    of << "ngram,count" << std::endl;
    for (size_t i = 0; i < k; ++i)
    {
      size_t index = countOrder[i];
      of << "0x" << std::hex
          << std::setw(2) << std::setfill('0') << (size_t) prefixes[3 * index]
          << std::setw(2) << std::setfill('0') << (size_t) prefixes[3 * index + 1]
          << std::setw(2) << std::setfill('0') << (size_t) prefixes[3 * index + 2]
          << "," << std::dec
          << prefixCounts[index] << std::endl;
    }

    std::cout << "Intermediate save time: " << stepC.toc() << "s." << std::endl;
  }

  //
  // 4-gramming and higher passes
  //
  for (size_t nIter = 4; nIter <= n; ++nIter)
  {
    stepC.tic();
    std::cout << "keepSize: " << keepSize << ", len " << (nIter - 1) << "\n";
    PackedByteTrie<uint32_t> trie(prefixes, prefixCounts, keepSize, nIter - 1);

    CountsArray<false> prefixedCounts(256 * keepSize, &trie);

    std::cout << "Trie construction time for length-" << (nIter - 1) << " prefixes: " << stepC.toc()
        << "s." << std::endl;

    // Take the pass over the data.
    stepC.tic();
    iter.reset();
    readerThreads = new SingleReaderThread*[threads];
    for (size_t i = 0; i < threads; ++i)
      readerThreads[i] = new SingleReaderThread(iter, nIter, i, verbosity);

    // Allow readers to initialize.
    usleep(500);

    PrefixSingleNgramThread** prefixNgramThreads =
        new PrefixSingleNgramThread*[threads];
    for (size_t i = 0; i < threads; ++i)
    {
      prefixNgramThreads[i] = new PrefixSingleNgramThread(*readerThreads[i],
          prefixedCounts, keepSize, &trie, nIter, i, verbosity);
    }

    // Allow ngram threads to initialize.
    usleep(1000);

    for (size_t i = 0; i < threads; ++i)
      prefixNgramThreads[i]->Finish();

    for (size_t i = 0; i < threads; ++i)
    {
      delete readerThreads[i];
      delete prefixNgramThreads[i];
    }

    std::cout << nIter << "-gram computation time: " << stepC.toc() << "s." << std::endl;

    // Compute top-k results.
    stepC.tic();
    free_hugepage<uint8_t>(prefixes, prefixMemState, (nIter - 1) * keepSize);
    keepSize = size_t(double(k) * (n == nIter ? 1.0 : overage));
    alloc_hugepage<uint8_t>(prefixes, prefixMemState, nIter * keepSize,
        "top-k computation");
    if (prefixCounts)
      delete[] prefixCounts;
    prefixCounts = new uint32_t[keepSize];

    keepSize = FindTopK(prefixedCounts, nIter, keepSize, prefixes, prefixCounts);
    std::cout << "Top-k computation time: " << stepC.toc() << "s (actually got "
        << keepSize << " prefixes)." << std::endl;

    if (saveIntermediate == 1 || n == nIter)
    {
      arma::Col<uint32_t> countsVec(prefixCounts, keepSize, false, true);
      arma::uvec countOrder = arma::sort_index(countsVec, "descend");

      std::ostringstream ofName;
      ofName << outputPrefix << "." << nIter << ".txt";
      std::fstream of(ofName.str(), std::fstream::out);
      of << "ngram,count" << std::endl;
      for (size_t i = 0; i < k; ++i)
      {
        size_t index = countOrder[i];
        of << "0x" << std::hex;
        for (size_t j = 0; j < nIter; ++j)
          of << std::setw(2) << std::setfill('0') << (size_t) prefixes[nIter * index + j];
        of << "," << std::dec << prefixCounts[index] << std::endl;
      }
    }
  }

  free_hugepage<uint8_t>(prefixes, prefixMemState, n * keepSize);
  delete[] prefixCounts;

  std::cout << "Total " << n << "-gram computation time: " << overallC.toc()
      << "s." << std::endl;
}
