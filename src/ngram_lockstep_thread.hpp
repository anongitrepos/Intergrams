// ngram_lockstep_thread.hpp: an NgramThread that is only responsible for a
// subset of the possible bytes.
#ifndef PNGRAM_NGRAM_LOCKSTEP_THREAD_HPP
#define PNGRAM_NGRAM_LOCKSTEP_THREAD_HPP

#include "lockstep_reader_thread.hpp"
#include <stdint.h>
#include "simd_util.hpp"

class WorkStealerThread;

/**
 * This thread is responsible for the lower 18 bits of any 3-gram that starts
 * with the first 6 bits equivalent to threadId.
 */
template<typename ReaderThreadType = LockstepReaderThread>
class NgramLockstepThread
{
 public:
  inline NgramLockstepThread(ReaderThreadType& reader,
                             const size_t threadId,
                             uint32_t* gram3Counts,
                             std::atomic_uint64_t& globalWaitingForChunk,
                             std::atomic<double>& globalFlushTime);
  inline ~NgramLockstepThread();

  inline NgramLockstepThread(NgramLockstepThread&& other) noexcept;

  inline void RunThread();

  constexpr static const size_t N = (1 << 18);
  constexpr static const size_t B = (N / 8);

  inline void ProcessChunkSIMD(const u512* ptr,
                               const size_t bytes,
                               uint8_t* targetBits);

 private:
  ReaderThreadType& reader;
  size_t threadId;
  uint8_t mask;
  uint32_t* gram3Counts;
  size_t gram3Offset;
  uint64_t mask8Bytes;
  u512 fullMask;
  u512 seqMasks; // Masks for detecting our prefix in an 8-byte sequence.

  // This mask strips the last 2 bits off every byte.
  constexpr static u512 prefixMask = {
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC,
      0xFCFCFCFCFCFCFCFC };

  constexpr static u512 lowBitSet = {
      0x0101010101010101,
      0x0101010101010101,
      0x0101010101010101,
      0x0101010101010101,
      0x0101010101010101,
      0x0101010101010101,
      0x0101010101010101,
      0x0101010101010101 };

  constexpr static u512 highBitSet = {
      0x8080808080808080,
      0x8080808080808080,
      0x8080808080808080,
      0x8080808080808080,
      0x8080808080808080,
      0x8080808080808080,
      0x8080808080808080,
      0x8080808080808080 };

  // These masks return the 3-grams for each of 8 byte offsets, with the leading
  // 6 bits stripped (because those are the prefixes, held in seqMasks), and
  // with the tailing 3 bits stripped (because those will indicate the bit index
  // in `bits` we have to use).
  //
  // When shifted right with `rightByteIndexShifts`, all values will be between
  // 0 and B.
  constexpr static u512 byteIndexMasks = {
      0x03FFF80000000000,
      0x0003FFF800000000,
      0x000003FFF8000000,
      0x00000003FFF80000,
      0x0000000003FFF800,
      0x000000000003FFF8,
      0x00000000000003FF,
      0x0000000000000003 };

  // This can be used to mask the last bytes of the last two 3-grams from the
  // *next* u512.
  constexpr static u512 nextByteIndexMasks = {
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0xF800000000000000,
      0xFFF8000000000000 };

  constexpr static i512 byteIndexShifts = { 43, 35, 27, 19, 11, 3, -5, -13 };
  constexpr static u512 rightByteIndexShifts = { 43, 35, 27, 19, 11, 3, 0, 0 };
  constexpr static u512 leftByteIndexShifts = { 0, 0, 0, 0, 0, 0, 5, 13 };

  constexpr static u512 rightNextByteIndexShifts = { 0, 0, 0, 0, 0, 0, 59, 51 };

  // These masks return the last 3 bits of each of the 3-grams for each of the 8
  // byte offsets.  These are used to index into `bits`.
  //
  // When shifted right with `rightBitValueShifts` all values will be between 0
  // and 7.
  constexpr static u512 bitValueMasks = {
      0x0000070000000000,
      0x0000000700000000,
      0x0000000007000000,
      0x0000000000070000,
      0x0000000000000700,
      0x0000000000000007,
      0x0000000000000000,
      0x0000000000000000 };

  // This can be used to mask the last 3 bits of each of the last two 3-grams
  // from the *next* u512.
  constexpr static u512 nextBitValueMasks = {
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000,
      0x0700000000000000,
      0x0007000000000000 };

  constexpr static i512 bitValueShifts = { 40, 32, 24, 16, 8, 0, -8, -16 };
  constexpr static u512 rightBitValueShifts = { 40, 32, 24, 16, 8, 0, 0, 0 };
  constexpr static u512 leftBitValueShifts = { 0, 0, 0, 0, 0, 0, 8, 16 };

  constexpr static u512 rightNextBitValueShifts = { 0, 0, 0, 0, 0, 0, 56, 48 };

  constexpr static u512 finalBitValueShifts = { 56, 48, 40, 32, 24, 16, 8, 0 };

  uint8_t bits[B];

  std::thread thread;
  size_t waitingForChunk;
  double flushTime;

  uint64_t bitValues;

  std::atomic_uint64_t& globalWaitingForChunk;
  std::atomic<double>& globalFlushTime;

  friend class WorkStealerThread;
};

#include "ngram_lockstep_thread_impl.hpp"

#endif
