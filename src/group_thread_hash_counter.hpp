#ifndef PNGRAM_GROUP_THREAD_HASH_COUNTER_HPP
#define PNGRAM_GROUP_THREAD_HASH_COUNTER_HPP

#include <bitset>
#include <atomic>
#include "counts_array.hpp"

template<size_t NumBitsets>
class GroupThreadHashCounter
{
 public:
  GroupThreadHashCounter(CountsArray& counts,
                         uint64_t* bitsets,
                         std::atomic_uint8_t* bitsetStatuses);

  ~GroupThreadHashCounter();

  // Get the ID of a bitset corresponding to the given file ID, or assign a new
  // bitset to the given file ID if it hasn't happened yet.  If a flush is
  // needed, this performs a flush first (so it may take a little while!).
  inline size_t GetBitsetId(const size_t fileId);

  // Same as above but don't allocate a new one if it's not found.
  inline size_t GetExistingBitsetId(const size_t fileId);

  // Count the 3-gram in `bytes` for the given bitset ID.
  inline void Set(const unsigned char* bytes, const size_t bitsetId);

  // Mark the bitset ID as completed and ready for flushing.
  inline void FinishBitset(const size_t bitsetId, const bool finish);

  // Flush everything until nothing more can be flushed.
  inline void ForceFlush();

  inline void Flush(const size_t index1);
  inline void Flush(const size_t index1, const size_t index2);
  inline void Flush(const size_t index1, const size_t index2,
                    const size_t index3);
  inline void Flush(const size_t index1, const size_t index2,
                    const size_t index3, const size_t index4);
  inline void Flush(const size_t index1, const size_t index2,
                    const size_t index3, const size_t index4,
                    const size_t index5);
  inline void Flush(const size_t index1, const size_t index2,
                    const size_t index3, const size_t index4,
                    const size_t index5, const size_t index6);
  inline void Flush(const size_t index1, const size_t index2,
                    const size_t index3, const size_t index4,
                    const size_t index5, const size_t index6,
                    const size_t index7);
  inline void Flush(const size_t index1, const size_t index2,
                    const size_t index3, const size_t index4,
                    const size_t index5, const size_t index6,
                    const size_t index7, const size_t index8);

  CountsArray& countsArray;
  uint64_t* bitsets;
  std::atomic_uint8_t* bitsetStatuses;
  size_t waitingForBitset;
  size_t waitingForAssignMutex;
  size_t flushes;
  size_t fileIds[NumBitsets];
  std::atomic_bool assignMutex;

  // statuses:
  //  0: available for use, all zeros
  //  1: being filled
  //  2: needs to be flushed
  //  3: flushing
  //  4: finished
};

#include "group_thread_hash_counter_impl.hpp"

#endif
