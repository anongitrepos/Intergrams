// compute_ngrams_group_parallel.cpp:
//
// Compute n-grams where a group of threads is dedicated to an individual
// reader.
#include "group_reader_thread.hpp"
#include "group_ngram_thread.hpp"
#include "alloc.hpp"
#include <armadillo>
#include <fstream>

constexpr const size_t NumBitsets = 4;

int main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " directory/ <n_readers> "
        << "<threads_per_group>" << std::endl;
    exit(1);
  }

  std::filesystem::path directory(argv[1]);
  const size_t numReaders = atoi(argv[2]);
  const size_t threadsPerGroup = atoi(argv[3]);
  DirectoryIterator iter({ directory }, false);

  arma::wall_clock c, c2;
  c.tic();
  c2.tic();

  CountsArray globalCounts;

  // Create all of the bitsets that we will use.
  alignas(64) uint64_t* bitsets;
  alloc_mem_state bitsetsMemState;
  alloc_hugepage<uint64_t>(bitsets, bitsetsMemState,
      NumBitsets * numReaders * 262144, "n-gramming");
  memset(bitsets, 0, sizeof(uint64_t) * NumBitsets * numReaders * 262144);

  std::atomic_uint8_t** bitsetStatuses = new std::atomic_uint8_t*[numReaders];
  for (size_t r = 0; r < numReaders; ++r)
  {
    bitsetStatuses[r] = new std::atomic_uint8_t[NumBitsets];
    memset(bitsetStatuses[r], 0, sizeof(std::atomic_uint8_t) * NumBitsets);
  }

  // Assemble the counters and the reader threads.
  GroupReaderThread<NumBitsets>** readerThreads =
      new GroupReaderThread<NumBitsets>*[numReaders];
  std::vector<GroupThreadHashCounter<NumBitsets>*> counters;
  for (size_t r = 0; r < numReaders; ++r)
  {
    counters.push_back(new GroupThreadHashCounter<NumBitsets>(globalCounts,
        (uint64_t*) (bitsets + 262144 * NumBitsets * r), bitsetStatuses[r]));

    readerThreads[r] = new GroupReaderThread<NumBitsets>(iter, *counters[r], 3,
        r * threadsPerGroup);
  }

  // Allow readers to initialize.
  usleep(100);

  // Initialize worker threads.
  std::vector<std::vector<GroupNgramThread<NumBitsets>*>> workThreads;
  for (size_t r = 0; r < numReaders; ++r)
  {
    workThreads.push_back(std::vector<GroupNgramThread<NumBitsets>*>());
    for (size_t t = 0; t < threadsPerGroup - 1; ++t)
    {
      workThreads[r].push_back(new GroupNgramThread<NumBitsets>(
          *readerThreads[r], *counters[r], 3, r * threadsPerGroup + 1 + t));
    }
  }

  // Allow ngram threads to initialize.
  usleep(1000);

  for (size_t r = 0; r < numReaders; ++r)
    for (size_t i = 0; i < threadsPerGroup - 1; ++i)
      workThreads[r][i]->Finish();

  std::cout << "GroupNgramThreads finished (" << c2.toc() << "s)." << std::endl;
  for (size_t r = 0; r < numReaders; ++r)
  {
    for (size_t t = 0; t < threadsPerGroup - 1; ++t)
      delete workThreads[r][t];

    delete readerThreads[r];
    delete counters[r];
    delete[] bitsetStatuses[r];
  }

  std::cout << "Total time: " << c.toc() << "s." << std::endl;

  std::cout << "Maximum count: " << globalCounts.Maximum() << ".\n";
  //std::fstream f("ngrams-out.csv", std::fstream::out);
  //for (size_t i = 0; i < 16777216; ++i)
  //{
  //  f << "0x" << std::hex << std::setw(6) << std::setfill('0') << i << ", "
  //      << std::dec << (size_t) globalCounts[i] << "\n";
  //}
  //globalCounts.Save("ngrams-out.csv");

  delete[] readerThreads;
  free_hugepage<uint64_t>(bitsets, bitsetsMemState,
      numReaders * NumBitsets * 262144);
}
