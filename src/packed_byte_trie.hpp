// packed_byte_trie.hpp: implementation of ByteTrie as a single object
#ifndef PNGRAM_PACKED_BYTE_TRIE_HPP
#define PNGRAM_PACKED_BYTE_TRIE_HPP

#include <stdint.h>
#include "alloc.hpp"

template<typename IndexType = uint32_t>
class PackedByteTrie
{
 public:
  PackedByteTrie(uint8_t* prefixes,
                 uint32_t* counts,
                 const size_t numPrefixes,
                 const size_t prefixLen);

  void CheckConsistency(
    uint8_t* prefixes,
    size_t& leafIndex,
    size_t& j,
    uint8_t* triePrefix,
    const size_t nodeIndex,
    const size_t depth) const;

  ~PackedByteTrie();

  size_t Search(const uint8_t* prefixes) const;

  size_t PrefixLen() const { return prefixLen; }

  //
  // Total trie size:
  //    N nodes,
  //    2 + 2 * sizeof(IndexType) bytes per node
  //
  // N will be greater than the number of prefixes, but in some kind of
  // logarithmic sense.
  //

  IndexType* children; // holds the index of the first child

  uint8_t* numChildren; // one per node in the trie
  uint8_t* bytes; // one per node in the trie

  size_t prefixLen;

  constexpr static size_t minChildrenForChildMap = 2;

  // holds the actual child indexes
  IndexType* childrenVector;

  alloc_mem_state childrenMemState;
  alloc_mem_state numChildrenMemState;
  alloc_mem_state bytesMemState;
  alloc_mem_state childrenVectorMemState;

  size_t numTotalNodes;
  size_t childrenVectorLen;

  std::tuple<size_t, size_t> ReorderPrefixes(uint8_t* prefixes,
                                             uint32_t* counts,
                                             const size_t numPrefixes,
                                             const size_t prefixLen,
                                             const size_t byteIndex) const;

  // The prefixes should all be sorted by this point.
  std::tuple<size_t, size_t> BuildTrie(
      const uint8_t* prefixes,
      const size_t numPrefixes,
      const size_t prefixLen,
      const size_t byteIndex,
      const size_t currentPos,
      const size_t childrenStartPos,
      const size_t currentChildrenVectorPos,
      size_t& leafIndex);
};

#include "packed_byte_trie_impl.hpp"

#endif
