// byte_trie.hpp: definition of ByteTrie, for quick lookup of n-grams.
#ifndef PNGRAM_BYTE_TRIE_HPP
#define PNGRAM_BYTE_TRIE_HPP

#include <stdint.h>
#include <unordered_map>
#include <iostream>
#include <bitset>
#include <string>
#include "alloc.hpp"

/**
 * The ByteTrie is a simple implementation of a trie tuned for byte n-gramming
 * and hashing tasks.
 *
 * The trie can be initialized as an empty trie or batch-constructed on a set of
 * n-grams or hashes.
 *
 * The leaves of the trie are used by the n-grammer in two ways:
 *  - to store exact counts of how often a given hash or n-gram occurs
 *  - to store the dimension index that corresponds to a given n-gram
 *
 * When used to store exact counts, the trie can be converted to dimension
 * indices, optionally filtering on the top k values found in the trie.
 */
class ByteTrie
{
 public:
  // Default constructor creates an empty trie.
  ByteTrie();

  ByteTrie(const uint8_t* prefixes,
           const size_t len,
           const size_t count,
           const size_t byteIndex = 0);

  // Copy and move constructors and operators.
  ByteTrie(const ByteTrie& other);
  ByteTrie(ByteTrie&& other);
  ByteTrie& operator=(const ByteTrie& other);
  ByteTrie& operator=(ByteTrie&& other);

  // Delete all memory owned by the ByteTrie.
  ~ByteTrie();

  // Set the value associated with a given n-gram to `value`.
  // This creates a trie node if necessary.
  void Set(const uint8_t* b, const size_t len, const uint32_t value);

  // Increment the value associated with a given n-gram.
  // If the n-gram doesn't exist, create the necessary trie nodes and set the value to 0.
  void Increment(const uint8_t* b, const size_t len);

  // Search the trie to see if a given byte sequence is in the trie.
  // If not, nullptr is returned.
  //
  // Note that `b` must be the same amount of bytes that the trie is built on.
  // If it isn't... a crash is almost certain to happen!
  const ByteTrie* Search(const uint8_t* b) const;
  ByteTrie* Search(const uint8_t* b);

  uint8_t Byte() const;
  bool IsLeaf() const;
  uint32_t Value() const;
  void Value(const uint32_t new_value);

  uint32_t Length() const; // Computes the number of bytes in a n-gram.  Note this is not cached!
  size_t TrieSize() const; // Computes the number of leaves in the trie.  Note this is not cached!

  /**
   * An iterator for keys in a ByteTrie.  This iterator should be used when
   * only the keys of the ByteTrie are important.  It does not return the
   * values held by each node of the ByteTrie.  For that, use ValueIterator.
   */
  class KeyIterator {
   public:
    KeyIterator(const ByteTrie& b);
    KeyIterator(); // marks it at the end

    KeyIterator(const KeyIterator& other);
    KeyIterator(KeyIterator&& other);
    KeyIterator& operator=(const KeyIterator& other);
    KeyIterator& operator=(KeyIterator&& other);

    ~KeyIterator();

    void operator++();
    const uint8_t* operator*() const;

    bool operator==(const KeyIterator& other) const;
    bool operator!=(const KeyIterator& other) const;

   private:
    size_t len;
    const ByteTrie** stack;
    uint8_t* childIds;
    uint8_t* bytes;
    bool atEnd;
  };

  // Get a KeyIterator pointing at the beginning of the trie.
  KeyIterator begin_keys() const;
  // Get a KeyIterator pointing past the end of the trie.
  KeyIterator end_keys() const;

  /**
   * An iterator for values in a ByteTrie.  This iterator should be used when
   * only the values of the ByteTrie are important.  Keys are not available.
   * If keys are needed, use KeyIterator.
   *
   * Values can be modified during iteration, and the DeleteAndStep() method
   * can be used to delete the current trie node and step the iterator
   * forward.
   */
  class ValueIterator
  {
   public:
    ValueIterator(ByteTrie& b);
    ValueIterator(); // marks it at the end

    ValueIterator(const ValueIterator& other);
    ValueIterator(ValueIterator&& other);
    ValueIterator& operator=(const ValueIterator& other);
    ValueIterator& operator=(ValueIterator&& other);

    ~ValueIterator();

    void operator++();
    uint32_t& operator*();

    bool operator==(const ValueIterator& other) const;
    bool operator!=(const ValueIterator& other) const;

   private:
    size_t len;
    ByteTrie** stack;
    uint8_t* childIds;
    bool atEnd;
  };

  // Get a ValueIterator pointing at the beginning of the trie.
  ValueIterator begin_values();
  // Get a ValueIterator pointing past the end of the trie.
  ValueIterator end_values() const;

  // Return a list of strings containing the n-grams in this trie, ordered by their associated value.
  // (This only works correctly if all values are distinct indices and not counts!)
  operator std::vector<std::string>() const;

  bool operator==(const ByteTrie& other) const;
  bool operator!=(const ByteTrie& other) const;

 private:
  void Build(const uint8_t* ngrams, const size_t len, const size_t count,
             const size_t byteIndex = 0, const size_t valIndex = 0);

  // Return the child associated with `byte`, if it exists; otherwise return
  // nullptr if there is no child associated with `byte`.
  ByteTrie* Step(uint8_t byte) const;

  // fullByteStr must have length 2 * n, and must be set before calling
  // Print() on the root.  i represents the byte position.
  void Print(std::ostream& os, std::string& fullByteStr, const size_t i) const;

  // Same as Print(), but the printed n-gram will be put into vector[value],
  // where `value` is the value member of the trie leaf.
  void PrintIntoVector(std::vector<std::string>& vector, std::string& fullByteStr, const size_t i) const;

  // Get the maximum value held by any child of this node.
  void FindMaxValue(uint32_t& maxValue) const;

  // The byte we are representing.  Note that for the root of the trie this is unused!
  uint8_t byte;
  uint16_t numChildren;

  // The actual value (the dimension in the dataset) corresponding to this
  // node in the true; only meaningful if the value is a leaf.
  uint32_t value;

  // nullptr if we are a leaf.
  ByteTrie* children;

  // nullptr if there are only a couple children; otherwise this can be used
  // for fast lookup of children instead of iterating over them.
  uint8_t* childMap;

  // When the trie node has more than this many children, childMap is used to
  // find children instead of manually iterating over them.  This may need to
  // be tuned per system.
  constexpr static size_t minChildrenForChildMap = 4;

  // Output all n-grams to a stream.
  // This prints one n-gram per line, in sorted order.
  friend std::ostream& operator<<(std::ostream& os, const ByteTrie& b);

  friend std::tuple<ByteTrie*, size_t, alloc_mem_state> pack_byte_trie(const ByteTrie& b);
  friend std::tuple<ByteTrie*, alloc_mem_state> copy_packed_byte_trie(const ByteTrie* b, const size_t num_bytes);
};

// Auxiliary functions to pack a ByteTrie into fully contiguous memory.
inline std::tuple<ByteTrie*, size_t, alloc_mem_state> pack_byte_trie(const ByteTrie& b);
inline std::tuple<ByteTrie*, alloc_mem_state> copy_packed_byte_trie(const ByteTrie* b, const size_t num_bytes);
inline void clean_packed_byte_trie(ByteTrie* b, const alloc_mem_state s, const size_t num_bytes);

#include "byte_trie_impl.hpp"

#endif
