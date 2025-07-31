#ifndef PNGRAM_WORK_STEALER_THREAD_HPP
#define PNGRAM_WORK_STEALER_THREAD_HPP

#include "lockstep_stealing_reader_thread.hpp"
#include "ngram_lockstep_thread.hpp"

class WorkStealerThread
{
 public:
  inline WorkStealerThread(LockstepStealingReaderThread& reader,
                           std::vector<NgramLockstepThread<LockstepStealingReaderThread>*>& ngramThreads);
  inline ~WorkStealerThread();

  inline void RunThread();

  constexpr static const size_t N = (1 << 18);
  constexpr static const size_t B = (N / 8);

 private:
  LockstepStealingReaderThread& reader;
  std::vector<NgramLockstepThread<LockstepStealingReaderThread>*>& ngramThreads;

  uint8_t bits[B];
  uint64_t waitingForChunkToSteal;

  std::thread thread;
};

#include "work_stealer_thread_impl.hpp"

#endif
