#include "reader_thread.hpp"
#include "ngram_thread.hpp"
#include "flush_thread.hpp"
#include <armadillo>
#include "alloc.hpp"

int main(int argc, char** argv)
{
  // This strategy uses a bunch of NgramThreads that each process individual
  // chunks to compute 3-grams.  It seems to have significant cache issues.
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " directory/ n_threads" << std::endl;
    exit(1);
  }

  arma::wall_clock c;

  std::atomic_uint32_t* gram3_counts;
  alloc_mem_state gram_counts_mem_state;
  alloc_hugepage<std::atomic_uint32_t>(gram3_counts, gram_counts_mem_state,
      16777216, "n-gramming");
  memset(gram3_counts, 0, sizeof(std::atomic_uint32_t) * 16777216);
  ThreadHashCounter* thread_counters =
      new ThreadHashCounter[ReaderThread::num_files];
  for (size_t i = 0; i < ReaderThread::num_files; ++i)
    thread_counters[i] = ThreadHashCounter();
  const size_t threads = atoi(argv[2]);

  c.tic();
  {
    // This will start getting chunks immediately.
    std::filesystem::path d(argv[1]);
    DirectoryIterator d_it({ d }, false);
    ReaderThread reader(d_it, 3);
    usleep(100); // this may not be enough??
    {
      // Create threads for flushing.
      FlushThread flusher1(reader, thread_counters, gram3_counts);
      FlushThread flusher2(reader, thread_counters, gram3_counts);
      FlushThread flusher3(reader, thread_counters, gram3_counts);
      FlushThread flusher4(reader, thread_counters, gram3_counts);
      FlushThread flusher5(reader, thread_counters, gram3_counts);
      FlushThread flusher6(reader, thread_counters, gram3_counts);
      FlushThread flusher7(reader, thread_counters, gram3_counts);
      FlushThread flusher8(reader, thread_counters, gram3_counts);

      // Fire off the thread to process the n-grams.
      {
        std::vector<NgramThread> ngrams;
        for (size_t t = 0; t < threads; ++t)
          ngrams.emplace_back(NgramThread(reader, thread_counters, 3));
      }
    }

    // This will only be exited when everything is done.
  }

  const double processing_time = c.toc();

  std::cout << "Computing 3-grams took " << processing_time << "s.\n";

  uint32_t max_count = 0;
  for (size_t i = 0; i < 16777216; ++i)
    max_count = std::max(max_count, (uint32_t) gram3_counts[i]);

  std::cout << "max count: " << max_count << "\n";

  free_hugepage<std::atomic_uint32_t>(gram3_counts, gram_counts_mem_state,
      16777216);
}
