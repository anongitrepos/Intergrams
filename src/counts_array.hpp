#ifndef PNGRAM_COUNTS_ARRAY_HPP
#define PNGRAM_COUNTS_ARRAY_HPP

#include "simd_util.hpp"
#include <mutex>
#include "alloc.hpp"
#include <string>
#include "packed_byte_trie.hpp"

template<bool FixedSize = true>
class CountsArray
{
 public:
  CountsArray();
  CountsArray(const size_t size, const PackedByteTrie<uint32_t>* prefixTrie);
  ~CountsArray();

  inline void Increment(const size_t index,
                        const uint64_t bits);

  inline void Increment(const size_t index,
                        const uint64_t bits1,
                        const uint64_t bits2);

  inline void Increment(const size_t index,
                        const uint64_t bits1,
                        const uint64_t bits2,
                        const uint64_t bits3);

  inline void Increment(const size_t index,
                        const uint64_t bits1,
                        const uint64_t bits2,
                        const uint64_t bits3,
                        const uint64_t bits4);

  inline void Increment(const size_t index,
                        const uint64_t bits1,
                        const uint64_t bits2,
                        const uint64_t bits3,
                        const uint64_t bits4,
                        const uint64_t bits5,
                        const uint64_t bits6,
                        const uint64_t bits7,
                        const uint64_t bits8);

  inline void Increment(const size_t index,
                        const uint64_t bits1,
                        const uint64_t bits2,
                        const uint64_t bits3,
                        const uint64_t bits4,
                        const uint64_t bits5,
                        const uint64_t bits6,
                        const uint64_t bits7,
                        const uint64_t bits8,
                        const uint64_t bits9,
                        const uint64_t bits10,
                        const uint64_t bits11,
                        const uint64_t bits12,
                        const uint64_t bits13,
                        const uint64_t bits14,
                        const uint64_t bits15,
                        const uint64_t bits16);

  uint32_t Maximum() const;

  void Save(const std::string& filename) const;

  uint32_t operator[](const size_t i) const;
  constexpr size_t Size() const { return FixedSize ? 16777216 : 16 * size; }
  size_t CopyPrefixes(uint8_t* prefixes,
                      const uint32_t minValForCopy,
                      size_t numWithMinValToCopy,
                      uint32_t* prefixCounts) const;

 private:
  u32_512* counts;
  alloc_mem_state countsMemState;
  std::mutex* mutexes;
  size_t size;
  const PackedByteTrie<uint32_t>* prefixTrie;
  uint8_t* prefixTrieKey;
  size_t prefixTrieLoc;

  void PrefixTrieIterate(uint8_t* prefixes,
                         size_t& leafIndex,
                         size_t& j,
                         uint8_t* triePrefix,
                         const uint32_t minValForCopy,
                         size_t& numWithMinValToCopy,
                         uint32_t* prefixCounts,
                         const size_t nodeIndex,
                         const size_t depth) const;
};

#include "counts_array_impl.hpp"

#endif
