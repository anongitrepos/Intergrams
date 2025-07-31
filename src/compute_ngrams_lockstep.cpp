#include "lockstep_reader_thread.hpp"
#include "ngram_lockstep_thread.hpp"
#include <armadillo>
#include "alloc.hpp"

int main(int argc, char** argv)
{
  // This strategy uses a bunch of LockstepNgramThreads to compute 3-grams.
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " directory/" << std::endl;
    exit(1);
  }

  arma::wall_clock c;

  uint32_t* gram3Counts;
  alloc_mem_state gramCountsMemState;
  alloc_hugepage<uint32_t>(gram3Counts, gramCountsMemState, 16777216,
      "n-gramming");
  memset(gram3Counts, 0, sizeof(uint32_t) * 16777216);

  std::atomic_uint64_t globalWaitingForChunk = 0;
  std::atomic<double> globalFlushTime = 0.0;

  c.tic();
  {
    // This will start getting chunks immediately.
    std::filesystem::path d(argv[1]);
    LockstepReaderThread reader(d, 3);
    usleep(100); // this may not be enough??
    {
      // Fire off the thread to process the n-grams.
      {
        std::vector<NgramLockstepThread<>*> ngrams;
        for (size_t t = 0; t < 64; ++t)
          ngrams.emplace_back(new NgramLockstepThread<>(reader, t, gram3Counts,
              globalWaitingForChunk, globalFlushTime));

        for (size_t t = 0; t < 64; ++t)
          delete ngrams[t];
      }
    }

    // This will only be exited when everything is done.
  }

  const double processing_time = c.toc();

  std::cout << "Computing 3-grams took " << processing_time << "s.\n";
  std::cout << "Average number of iterations by NgramLockstepThreads: "
      << (globalWaitingForChunk / 64) << ".\n";
  std::cout << "Average flush time per thread: " << (globalFlushTime / 64.0)
      << ".\n";

  uint32_t max_count = 0;
  for (size_t i = 0; i < 16777216; ++i)
    max_count = std::max(max_count, gram3Counts[i]);

  std::cout << "Max count: " << max_count << "\n";

  free_hugepage<uint32_t>(gram3Counts, gramCountsMemState, 16777216);
}
