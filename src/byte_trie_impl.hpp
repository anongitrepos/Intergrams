#ifndef PNGRAM_BYTE_TRIE_IMPL_HPP
#define PNGRAM_BYTE_TRIE_IMPL_HPP

#include "byte_trie.hpp"
#include <bitset>
#include <sstream>
#include <cstring>
#include <stack>
#include <queue>
#include <algorithm>
#include <iostream>

/**
 * Convert the byte `c` to its 2-character hex string representation, storing
 * those two characters in ngram_str[2 * i] and ngram_str[2 * i + 1].  You are
 * responsible for ensuring that `(2 * i + 1) < ngram_str.size()`.  This method
 * does not check.
 */
constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
void byte_to_str_pair(const char c, const size_t i, std::string& ngramStr)
{
  ngramStr[2 * i] = hexmap[(0xf0 & c) >> 4];
  ngramStr[2 * i + 1] = hexmap[0x0f & c];
}

// Implementation inlined so the compiler can make smart optimizations (...we hope).

inline ByteTrie::ByteTrie() :
    byte(0),
    numChildren(0),
    children(nullptr),
    childMap(nullptr),
    value(0)
{
  // Nothing to do.
}

inline ByteTrie::ByteTrie(const uint8_t* prefixes,
                          const size_t len,
                          const size_t count,
                          const size_t byteIndex) :
    byte(0),
    numChildren(0),
    children(nullptr),
    childMap(nullptr),
    value(0)
{
  Build(prefixes, len, count, byteIndex);
}

inline ByteTrie::ByteTrie(const ByteTrie& other) :
    numChildren(0),
    children(nullptr),
    childMap(nullptr)
{
  operator=(other);
}

inline ByteTrie::ByteTrie(ByteTrie&& other) :
    numChildren(0),
    children(nullptr),
    childMap(nullptr)
{
  operator=(std::move(other));
}

inline ByteTrie& ByteTrie::operator=(const ByteTrie& other)
{
  if (this != &other)
  {
    delete[] this->children;
    delete[] this->childMap;
    this->children = nullptr;
    this->childMap = nullptr;

    this->byte = other.byte;
    this->numChildren = other.numChildren;
    this->value = other.value;

    if (other.childMap)
    {
      // Copy the child map.
      this->childMap = new uint8_t[256];
      memcpy(this->childMap, other.childMap, 256);
    }

    if (this->numChildren > 0)
    {
      this->children = new ByteTrie[this->numChildren];
      for (size_t i = 0; i < this->numChildren; ++i)
          this->children[i] = other.children[i];
    }
  }

  return *this;
}

inline ByteTrie& ByteTrie::operator=(ByteTrie&& other)
{
  if (this != &other)
  {
    delete[] this->children;
    delete[] this->childMap;

    this->byte = std::move(other.byte);
    this->children = std::move(other.children);
    this->numChildren = std::move(other.numChildren);
    this->childMap = std::move(other.childMap);
    this->value = std::move(other.value);

    other.byte = 0;
    other.children = nullptr;
    other.numChildren = 0;
    other.childMap = nullptr;
    other.value = 0;
  }

  return *this;
}

inline ByteTrie::~ByteTrie()
{
  delete[] this->children;
  delete[] this->childMap;
}

inline void ByteTrie::Build(const uint8_t* prefixes,
                            const size_t len,
                            const size_t count,
                            const size_t byteIndex,
                            const size_t valOffset)
{
  if (count == 0)
    return;

  if (byteIndex >= len)
  {
    std::ostringstream oss;
    oss << "ByteTrie::ByteTrie(): invalid byteIndex (" << byteIndex
        << "); cannot be the last byte or later of n-grams (with n = "
        << len << ")!";
    throw std::invalid_argument(oss.str());
  }


  // Collect the set of bytes that we do have.
  std::bitset<256> hasBytes;
  for (size_t i = 0; i < count; ++i)
  {
    // Check the correct byte of the uint32_t.
    const uint8_t b = *((uint8_t*) &prefixes[i * len] + byteIndex);
    hasBytes[(size_t) b] = true;
  }

  // Now create the correct number of children.
  this->children = new ByteTrie[hasBytes.count()];
  this->numChildren = hasBytes.count();
  if (this->numChildren > this->minChildrenForChildMap)
  {
    // If this is true, we have enough children that we think it'll be faster to
    // do a look-up for each byte than to iterate over all the children.  This
    // is probably true when byteIndex is 0 or 1 (or small).
    this->childMap = new uint8_t[256];
    // Set all child map values to an invalid child.
    memset((void*) this->childMap, this->numChildren, 256);
  }

  // Set the byte for each child.
  size_t currentChild = 0;
  for (size_t b = 0; (b < 256) && (currentChild < this->numChildren); ++b)
  {
    if (hasBytes[(size_t) b])
    {
      this->children[currentChild].byte = (uint8_t) b;
      // Set the map value if we are building a child map.
      if (this->childMap)
        this->childMap[(size_t) b] = currentChild;
      ++currentChild;
    }
  }

  // Iterate over the n-grams to send the correct n-grams each direction.
  // Note that if we are looking at the second-to-last n-gram, we actually
  // have to create all the children as leaves.
  if (byteIndex == len - 1)
  {
    // The next level of the trie will all be leaves.  So, just extract the
    // dimension index and set that as a value.  Also, add a sanity check,
    // because we should not be seeing any particular byte twice.
    std::bitset<256> seenBytes;
    for (size_t i = 0; i < count; ++i)
    {
      const uint8_t byte = *((uint8_t*) &prefixes[i * len] + byteIndex);
      if (seenBytes[(size_t) byte])
      {
        std::ostringstream oss;
        oss << "ByteTrie::ByteTrie(): while assembling trie level " << byteIndex
            << ", encountered byte " << std::hex << (size_t) byte
            << " more than once!";
        throw std::runtime_error(oss.str());
      }

      // At this point, this level of the trie is complete, so we can use
      // Step() to find the right child.
      ByteTrie* child = Step(byte);
      child->value = valOffset + i;
    }
  }
  else
  {
    // The next level of the trie is not leaves, so construct subsets of the
    // n-grams and recursively build.
    size_t usedPrefixes = valOffset;
    for (size_t i = 0; i < this->numChildren; ++i)
    {
      const uint8_t b = this->children[i].byte;
      // Count the number of bytes we will need.  TODO: this could be optimized
      size_t childNgramCount = 0;
      for (size_t i = 0; i < count; ++i)
      {
        const uint8_t pb = *((uint8_t*) &prefixes[i * len] + byteIndex);
        if (pb == b)
          ++childNgramCount;
      }

      uint8_t* childNgrams = new uint8_t[len * childNgramCount];
      memset(childNgrams, 0, sizeof(uint8_t) * len * childNgramCount);
      size_t used = 0;
      for (size_t i = 0; i < count; ++i)
      {
        uint32_t new_p = 0;
        const uint8_t pb = *((uint8_t*) &prefixes[i * len] + byteIndex);
        if (pb == b)
        {
          memcpy(childNgrams + used * len, &prefixes[i * len],
              sizeof(uint8_t) * len);
          ++used;
        }
      }

      this->children[i].Build(childNgrams, len, childNgramCount, byteIndex + 1,
          usedPrefixes);
      usedPrefixes += childNgramCount;
    }
  }
}

inline void ByteTrie::Set(const uint8_t* b, const size_t len, const uint32_t value)
{
  if (len == 0)
  {
    // If we are looking at the last byte, set the value and we are done.
    this->value = value;
  }
  else
  {
    // If we are not looking at the last byte, we must first see if the child
    // exists.
    ByteTrie* child = Step(b[0]);
    if (child != nullptr)
    {
      child->Set(b + 1, len - 1, value);
    }
    else
    {
      throw std::runtime_error("prefix does not exist!");
    }
  }
}

inline void ByteTrie::Increment(const uint8_t* b, const size_t len)
{
  if (len == 0)
  {
    // If we are looking at the last byte, just increment and we are done.
    ++this->value;
  }
  else
  {
    // If we are not looking at the last byte, we must first see if the child
    // exists.
    ByteTrie* child = Step(b[0]);
    if (child != nullptr)
    {
      child->Increment(b + 1, len - 1);
    }
    else
    {
      throw std::runtime_error("prefix does not exist!");
    }
  }
}

inline ByteTrie* ByteTrie::Step(const uint8_t byte) const
{
  // First, see if we can simply look up the child.  This will happen when the
  // number of children this node has is high.
  if (this->childMap)
  {
    const size_t childIndex = (size_t) this->childMap[byte];
    if (childIndex >= this->numChildren)
      return nullptr;
    else
      return &this->children[childIndex];
  }
  else
  {
    // If we have no child map, we have to just iterate over all the children
    // manually.  This is faster when there are only a couple children.
    for (size_t i = 0; i < this->numChildren; ++i)
    {
      if (byte == this->children[i].Byte())
      {
        return &this->children[i];
      }
      else if (byte < this->children[i].Byte())
      {
        // We've already stepped past the byte of interest, so we can't possibly
        // have the right child.
        return nullptr;
      }
    }

    return nullptr;
  }
}

inline const ByteTrie* ByteTrie::Search(const uint8_t* b) const
{
  // Check for an empty trie.
  if (this->numChildren == 0)
    return nullptr;

  // The implementation is fully inlined manually for speed.
  const ByteTrie* n = this;
  while (n != nullptr)
  {
    // Find the next step.
    if (n->numChildren == 0)
    {
      return n;
    }
    else
    {
      // Find the right child.  This is a manual inlining of Step().

      // First, see if we can simply look up the child.  This will happen when
      // the number of children this node has is high.
      if (n->childMap)
      {
        const size_t childIndex = (size_t) n->childMap[*b];
        if (childIndex >= n->numChildren)
        {
          return nullptr; // Child not found---so the search fails.
        }
        else
        {
          n = &n->children[childIndex];
          ++b;
        }
      }
      else
      {
        // If we have no child map, we have to just iterate over all the
        // children manually.  This is faster when there are only a couple
        // children.
        size_t i = 0;
        const uint8_t* old_b = b;
        for ( ; i < n->numChildren; ++i)
        {
          if (*b == n->children[i].byte)
          {
            n = &n->children[i];
            // Prefetch both the new node's children and child map.
            #if !defined(_MSC_VER) // MSVC doesn't parse the parentheses below properly, and doesn't support this anyway.
            #if defined(__GNUC__) || (defined(__has_builtin) && __has_builtin(__builtin_prefetch))
            __builtin_prefetch(n->children, 1, 0);
            if (n->childMap)
              __builtin_prefetch(n->childMap, 1, 0);
            #endif
            #endif

            ++b;
            break;
          }
          else if (*b < n->children[i].byte)
          {
            // We've already stepped past the byte of interest, so we can't
            // possibly have the right child.
            return nullptr;
          }
        }

        if (b == old_b)
          return nullptr; // Child not found--so the search fails.
      }
    }
  }

  // We shouldn't get here.
  return nullptr;
}

inline ByteTrie* ByteTrie::Search(const uint8_t* b)
{
  // Call const version...
  const ByteTrie* result = ((const ByteTrie*) this)->Search(b);
  // ...but then change the result pointer to non-const.
  return (ByteTrie*) result;
}

inline uint8_t ByteTrie::Byte() const
{
  return this->byte;
}

inline bool ByteTrie::IsLeaf() const
{
  return (this->children == nullptr);
}

inline uint32_t ByteTrie::Value() const
{
  return this->value;
}

inline void ByteTrie::Value(const uint32_t value)
{
  this->value = value;
}

inline uint32_t ByteTrie::Length() const
{
  // Compute the size of each n-gram in the trie.
  uint32_t ngramLen = 0;
  const ByteTrie* n = this;
  while (n->numChildren > 0)
  {
    n = &n->children[0];
    ++ngramLen;
  }

  return ngramLen;
}

inline size_t ByteTrie::TrieSize() const
{
  if (IsLeaf())
    return 1;

  size_t result = 0;
  for (size_t i = 0; i < this->numChildren; ++i)
    result += this->children[i].TrieSize();
  return result;
}

inline void ByteTrie::Print(std::ostream& os, std::string& fullByteStr, const size_t i) const
{
  if (IsLeaf())
  {
    // If we are a leaf, simply print the prefix and then our byte..
    os << fullByteStr << std::endl;
  }
  else
  {
    // If we are a node in the tree, we have to recurse to each of our children.
    for (size_t j = 0; j < numChildren; ++j)
    {
      // Put the child byte into the string.
      byte_to_str_pair(children[j].byte, i, fullByteStr);
      this->children[j].Print(os, fullByteStr, i + 1);
    }
  }
}

// Convert all n-grams to a vector of strings.  This preserves the ordering (the
// values held by the trie leaves).
inline ByteTrie::operator std::vector<std::string>() const
{
  // First find the maximum index across all leaves, so that we know how many
  // elements to allocate in the vector.
  uint32_t numNgrams = 0;
  // We are cheating with the const-cast but we promise to not modify any values.
  ValueIterator v_it = ((ByteTrie*) this)->begin_values();
  while (v_it != end_values())
  {
    numNgrams = std::max(numNgrams, *v_it);
    ++v_it;
  }
  numNgrams++; // (Since we need one more capacity than we have maximum value.)

  // Now allocate the vector.
  std::vector<std::string> result(numNgrams, std::string());
  std::string fullByteStr(2 * Length(), '0');

  // Traverse the tree again, turning each n-gram into a string.
  PrintIntoVector(result, fullByteStr, 0);

  return result;
}

inline void ByteTrie::PrintIntoVector(std::vector<std::string>& ngrams,
                                      std::string& fullByteStr,
                                      const size_t i) const
{
  if (IsLeaf())
  {
    // We are done assembling the bytes, so now put them into the right place.
    // (This makes a copy.)
    ngrams[value] = fullByteStr;
  }
  else
  {
    // If we are a node in the tree, we need to recurse to each of our children.
    for (size_t j = 0; j < this->numChildren; ++j)
    {
      // Put the child's byte into the string.
      byte_to_str_pair(this->children[j].byte, i, fullByteStr);
      this->children[j].PrintIntoVector(ngrams, fullByteStr, i + 1);
    }
  }
}

// Output all n-grams to a stream.
// This prints one n-gram per line, in sorted order (assuming that dimensions
// are assigned).
inline std::ostream& operator<<(std::ostream& os, const ByteTrie& b)
{
  std::vector<std::string> ngrams(b);
  for (const std::string& str : ngrams)
    os << str << std::endl;
  return os;
}

inline bool ByteTrie::operator==(const ByteTrie& other) const
{
  if (this == &other)
    return true;

  if (this->byte != other.byte || this->numChildren != other.numChildren ||
      this->value != other.value)
    return false;

  for (size_t i = 0; i < this->numChildren; ++i)
    if (this->children[i] != other.children[i])
      return false;

  if (this->childMap != nullptr)
  {
    if (other.childMap == nullptr)
      return false;

    for (size_t i = 0; i < 256; ++i)
      if (this->childMap[i] != other.childMap[i])
        return false;
  }

  return true;
}

inline bool ByteTrie::operator!=(const ByteTrie& other) const
{
  return !(*this == other);
}

inline ByteTrie::KeyIterator::KeyIterator(const ByteTrie& b) :
    len(b.Length()),
    stack(new const ByteTrie*[len + 1]),
    childIds(new uint8_t[len]),
    bytes(new uint8_t[len]),
    atEnd(false)
{
  // Edge case: if the trie is empty, we are at the end.
  if (b.IsLeaf())
  {
    atEnd = true;
    return;
  }

  // Initialize root of stack and traverse down to first leaf.
  this->stack[0] = &b;
  size_t stackPos = 0;
  while (!this->stack[stackPos]->IsLeaf())
  {
    ++stackPos;
    this->stack[stackPos] = &this->stack[stackPos - 1]->children[0];
    // Set byte of child.
    this->bytes[stackPos - 1] = this->stack[stackPos]->byte;
  }
  // All child ids start at 0.
  memset(this->childIds, 0, len);
}

inline ByteTrie::KeyIterator::KeyIterator() :
    len(0),
    stack(nullptr),
    childIds(nullptr),
    bytes(nullptr),
    atEnd(true)
{
  // Nothing to do.
}

inline ByteTrie::KeyIterator::KeyIterator(const ByteTrie::KeyIterator& other) :
    ByteTrie::KeyIterator()
{
  operator=(other);
}

inline ByteTrie::KeyIterator::KeyIterator(ByteTrie::KeyIterator&& other) :
    ByteTrie::KeyIterator()
{
  operator=(std::move(other));
}

inline ByteTrie::KeyIterator& ByteTrie::KeyIterator::operator=(
    const ByteTrie::KeyIterator& other)
{
  if (this != &other)
  {
    delete[] this->stack;
    delete[] this->childIds;
    delete[] this->bytes;

    this->len = other.len;
    this->atEnd = other.atEnd;
    if (!this->atEnd)
    {
      this->stack = new const ByteTrie*[this->len + 1];
      this->childIds = new uint8_t[this->len];
      this->bytes = new uint8_t[this->len];

      memcpy(this->stack, other.stack, sizeof(const ByteTrie*) *
          (this->len + 1));
      memcpy(this->childIds, other.childIds, sizeof(uint8_t) * this->len);
      memcpy(this->bytes, other.bytes, sizeof(uint8_t) * this->len);
    }
    else
    {
      this->stack = nullptr;
      this->childIds = nullptr;
      this->bytes = nullptr;
    }
  }

  return *this;
}

inline ByteTrie::KeyIterator& ByteTrie::KeyIterator::operator=(
    ByteTrie::KeyIterator&& other)
{
  if (this != &other)
  {
    delete[] this->stack;
    delete[] this->childIds;
    delete[] this->bytes;

    this->len = other.len;
    this->stack = other.stack;
    this->childIds = other.childIds;
    this->bytes = other.bytes;
    this->atEnd = other.atEnd;

    other.len = 0;
    other.stack = nullptr;
    other.childIds = nullptr;
    other.bytes = nullptr;
    other.atEnd = true;
  }

  return *this;
}

inline ByteTrie::KeyIterator::~KeyIterator()
{
  delete[] stack;
  delete[] childIds;
  delete[] bytes;
}

inline void ByteTrie::KeyIterator::operator++()
{
  // We are currently looking at a leaf in the trie.  We have to go back up
  // until we find a suitable parent where we were not the last child.
  size_t stackPos = len;
  while ((stackPos > 0) && (this->childIds[stackPos - 1] >=
      this->stack[stackPos - 1]->numChildren - 1))
    --stackPos;

  // If we are out of children entirely, we're done.
  if ((stackPos == 0) && (this->childIds[0] >= this->stack[0]->numChildren - 1))
  {
    this->atEnd = true;
    return;
  }

  // Now increment the child index we stopped at and recurse back down.
  this->childIds[stackPos - 1]++;
  this->stack[stackPos] = &this->stack[stackPos - 1]->children[
      this->childIds[stackPos - 1]];
  this->bytes[stackPos - 1] = this->stack[stackPos]->byte;
  while (!this->stack[stackPos]->IsLeaf())
  {
    ++stackPos;
    this->childIds[stackPos - 1] = 0;
    this->stack[stackPos] = &this->stack[stackPos - 1]->children[0];
    // Set byte of child.
    this->bytes[stackPos - 1] = this->stack[stackPos]->byte;
  }
}

inline const uint8_t* ByteTrie::KeyIterator::operator*() const
{
  return bytes;
}

inline bool ByteTrie::KeyIterator::operator==(
    const ByteTrie::KeyIterator& other) const
{
  // Quick check to see if we are both end_keys().
  if (this->atEnd && other.atEnd)
      return true;

  // Now check the byte we are looking at.
  if (this->len != other.len)
      return false;

  // Check the state of the stack and bytes.
  for (size_t i = 0; i < this->len; ++i)
  {
    if (this->stack[i] != other.stack[i])
      return false;

    if (this->childIds[i] != other.childIds[i])
      return false;

    if (this->bytes[i] != other.bytes[i])
      return false;
  }

  return (this->stack[this->len] == other.stack[this->len]);
}

inline bool ByteTrie::KeyIterator::operator!=(
    const ByteTrie::KeyIterator& other) const
{
  return !(*this == other);
}

inline ByteTrie::ValueIterator::ValueIterator(ByteTrie& b) :
    len(b.Length()),
    stack(new ByteTrie*[len + 1]),
    childIds(new uint8_t[len]),
    atEnd(false)
{
  // Edge case: if the trie is empty, we are at the end.
  if (b.IsLeaf())
  {
    atEnd = true;
    return;
  }

  // Edge case: if the trie is empty, we are at the end.
  if (b.IsLeaf())
  {
    atEnd = true;
    return;
  }

  // Initialize root of stack and traverse down to first leaf.
  this->stack[0] = &b;
  size_t stackPos = 0;
  while (!this->stack[stackPos]->IsLeaf())
  {
    ++stackPos;
    this->stack[stackPos] = &this->stack[stackPos - 1]->children[0];
  }
  // All child ids start at 0.
  memset(this->childIds, 0, len);
}

inline ByteTrie::ValueIterator::ValueIterator() :
    len(0),
    stack(nullptr),
    childIds(nullptr),
    atEnd(true)
{
  // Nothing to do.
}

inline ByteTrie::ValueIterator::~ValueIterator()
{
  delete[] stack;
  delete[] childIds;
}

inline ByteTrie::ValueIterator::ValueIterator(const ByteTrie::ValueIterator& other) :
    ByteTrie::ValueIterator()
{
  operator=(other);
}

inline ByteTrie::ValueIterator::ValueIterator(ByteTrie::ValueIterator&& other) :
    ByteTrie::ValueIterator()
{
  operator=(std::move(other));
}

inline ByteTrie::ValueIterator& ByteTrie::ValueIterator::operator=(
    const ByteTrie::ValueIterator& other)
{
  if (this != &other)
  {
    delete[] this->stack;
    delete[] this->childIds;

    this->len = other.len;
    this->atEnd = other.atEnd;
    if (!this->atEnd)
    {
      this->stack = new ByteTrie*[this->len + 1];
      this->childIds = new uint8_t[this->len];

      memcpy(this->stack, other.stack, sizeof(ByteTrie*) * (this->len + 1));
      memcpy(this->childIds, other.childIds, sizeof(uint8_t) * this->len);
    }
    else
    {
      this->stack = nullptr;
      this->childIds = nullptr;
    }
  }

  return *this;
}

inline ByteTrie::ValueIterator& ByteTrie::ValueIterator::operator=(
    ByteTrie::ValueIterator&& other)
{
  if (this != &other)
  {
    delete[] this->stack;
    delete[] this->childIds;

    this->len = other.len;
    this->stack = other.stack;
    this->childIds = other.childIds;
    this->atEnd = other.atEnd;

    other.len = 0;
    other.stack = nullptr;
    other.childIds = nullptr;
    other.atEnd = true;
  }

  return *this;
}

inline void ByteTrie::ValueIterator::operator++()
{
  // We are currently looking at a leaf in the trie.  We have to go back up
  // until we find a suitable parent where we were not the last child.
  size_t stackPos = len;
  while ((stackPos > 0) && (this->childIds[stackPos - 1] >=
      this->stack[stackPos - 1]->numChildren - 1))
    --stackPos;

  // If we are out of children entirely, we're done.
  if ((stackPos == 0) && (this->childIds[0] >= this->stack[0]->numChildren - 1))
  {
    this->atEnd = true;
    return;
  }

  // Now increment the child index we stopped at and recurse back down.
  this->childIds[stackPos - 1]++;
  this->stack[stackPos] = &this->stack[stackPos - 1]->children[
      this->childIds[stackPos - 1]];
  while (!this->stack[stackPos]->IsLeaf())
  {
    ++stackPos;
    this->childIds[stackPos - 1] = 0;
    this->stack[stackPos] = &this->stack[stackPos - 1]->children[0];
  }
}

inline uint32_t& ByteTrie::ValueIterator::operator*()
{
  return stack[len]->value;
}

inline bool ByteTrie::ValueIterator::operator==(
    const ByteTrie::ValueIterator& other) const
{
  // Quick check to see if we are both end_values().
  if (this->atEnd && other.atEnd)
  {
    return true;
  }
  else if (this->atEnd != other.atEnd)
  {
    // One is at the end and the other isn't, so terminate the check early.
    return false;
  }

  // Now check the byte we are looking at.
  if (this->len != other.len)
    return false;

  // Check the state of the stack and bytes.
  for (size_t i = 0; i < this->len; ++i)
  {
    if (this->stack[i] != other.stack[i])
      return false;

    if (this->childIds[i] != other.childIds[i])
      return false;
  }

  return (this->stack[this->len] == other.stack[this->len]);
}

inline bool ByteTrie::ValueIterator::operator!=(
    const ByteTrie::ValueIterator& other) const
{
  return !(*this == other);
}

inline ByteTrie::KeyIterator ByteTrie::begin_keys() const
{
  return KeyIterator(*this);
}

inline ByteTrie::KeyIterator ByteTrie::end_keys() const
{
  return KeyIterator();
}

inline ByteTrie::ValueIterator ByteTrie::begin_values()
{
  return ValueIterator(*this);
}

inline ByteTrie::ValueIterator ByteTrie::end_values() const
{
  return ValueIterator();
}

inline std::tuple<ByteTrie*, size_t, alloc_mem_state> pack_byte_trie(
    const ByteTrie& b)
{
  // Count the memory we need to allocate.
  // Manually count the number of child maps.
  std::stack<const ByteTrie*> stack;
  stack.push(&b);
  size_t numNodes = 0;
  size_t numChildMaps = 0;
  while (!stack.empty())
  {
    const ByteTrie* n = stack.top();
    stack.pop();
    if (n->childMap)
      ++numChildMaps;
    ++numNodes;

    for (size_t i = 0; i < n->numChildren; ++i)
      stack.push(&n->children[i]);
  }

  // Allocate memory for everything.
  uint8_t* trieMemory;
  alloc_mem_state s;
  alloc_hugepage<uint8_t>(trieMemory, s,
      sizeof(ByteTrie) * numNodes + 256 * numChildMaps, "packing ByteTrie");
  // child maps are held at the end of memory
  uint8_t* childMapMemory = trieMemory + sizeof(ByteTrie) * numNodes;
  std::queue<const ByteTrie*> queue;
  queue.push(&b);
  size_t currentNode = 0;
  size_t currentChildMap = 0;
  while (!queue.empty())
  {
    const ByteTrie* n = queue.front();
    queue.pop();
    ByteTrie* newN = reinterpret_cast<ByteTrie*>(trieMemory +
        sizeof(ByteTrie) * currentNode);
    // Manually initialize all the members of the memory (which currently
    // are all invalid).
    newN->byte = n->byte;
    newN->numChildren = n->numChildren;
    newN->value = n->value;
    newN->children = nullptr;
    newN->childMap = nullptr;

    if (n->childMap)
    {
      // Copy the child map to the new location.
      newN->childMap = childMapMemory + 256 * currentChildMap;
      memcpy(newN->childMap, n->childMap, 256);
      ++currentChildMap;
    }

    if (n->children)
    {
      // Point the children to the right place.  The first child will be
      // at the end of the queue.
      newN->children = reinterpret_cast<ByteTrie*>(trieMemory +
          (currentNode + 1 + queue.size()) * sizeof(ByteTrie));
    }

    for (size_t i = 0; i < n->numChildren; ++i)
      queue.push(&n->children[i]);

    ++currentNode;
  }

  return std::make_tuple(reinterpret_cast<ByteTrie*>(trieMemory),
      sizeof(ByteTrie) * numNodes + 256 * numChildMaps, s);
}

inline std::tuple<ByteTrie*, alloc_mem_state> copy_packed_byte_trie(
    const ByteTrie* b, const size_t bytes)
{
  uint8_t* trieMemory;
  alloc_mem_state s;
  alloc_hugepage<uint8_t>(trieMemory, s, bytes, "copying packed ByteTrie");
  memcpy(trieMemory, reinterpret_cast<const uint8_t*>(b), bytes);

  // Now we have to iterate over the trie and set all of its pointers correctly.
  // Since the original object is all self-contained within its own memory, and
  // this object is allocated the same way, we can just update all the pointers
  // to point to the same offset within the block of allocated memory.
  std::stack<ByteTrie*> stack;
  stack.push(reinterpret_cast<ByteTrie*>(trieMemory));
  while (!stack.empty())
  {
    ByteTrie* n = stack.top();
    stack.pop();

    if (n->children)
    {
      size_t offset = reinterpret_cast<const uint8_t*>(n->children) -
          reinterpret_cast<const uint8_t*>(b);
      n->children = reinterpret_cast<ByteTrie*>(trieMemory + offset);
    }

    if (n->childMap)
    {
      size_t offset = reinterpret_cast<const uint8_t*>(n->childMap) -
          reinterpret_cast<const uint8_t*>(b);
      n->childMap = trieMemory + offset;
    }

    for (size_t i = 0; i < n->numChildren; ++i)
      stack.push(&n->children[i]);
  }

  return std::make_tuple(reinterpret_cast<ByteTrie*>(trieMemory), s);
}

inline void clean_packed_byte_trie(ByteTrie* b,
                                   const alloc_mem_state s,
                                   const size_t bytes)
{
  uint8_t* bMem = reinterpret_cast<uint8_t*>(b);
  free_hugepage<uint8_t>(bMem, s, bytes);
}

#endif
