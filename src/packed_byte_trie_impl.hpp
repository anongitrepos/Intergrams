// packed_byte_trie_impl.hpp
#ifndef PACKED_BYTE_TRIE_IMPL_HPP
#define PACKED_BYTE_TRIE_IMPL_HPP

#include <cstring>
#include <algorithm>

template<typename IndexType>
PackedByteTrie<IndexType>::PackedByteTrie(uint8_t* prefixes,
                                          uint32_t* counts,
                                          const size_t numPrefixes,
                                          const size_t prefixLen) :
    prefixLen(prefixLen)
{
  if (numPrefixes == 0)
    throw std::runtime_error("Cannot build empty PackedByteTrie!");

  //std::cout << "prefixes before:\n";
  //for (size_t i = 0; i < numPrefixes; ++i)
  //{
  //  std::cout << " - 0x";
  //  for (size_t j = 0; j < prefixLen; ++j)
  //    std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) prefixes[i *
//prefixLen + j];
//    std::cout << ": " << std::dec << (size_t) counts[i] << "\n";
//  }

  // First we need a count of how many nodes we'll be using.
  // To do this, we'll need to order by counts.
  std::tuple<size_t, size_t> result = ReorderPrefixes(prefixes, counts,
      numPrefixes, prefixLen, 0);
  numTotalNodes = 1 + std::get<0>(result);
  childrenVectorLen = std::get<1>(result);

//  std::cout << "total number of nodes: " << numTotalNodes << "\n";
//  std::cout << "children vector size: " << childrenVectorLen << "\n";
//  std::cout << "prefixes after:\n";
//  for (size_t i = 0; i < numPrefixes; ++i)
//  {
//    std::cout << " - 0x";
//    for (size_t j = 0; j < prefixLen; ++j)
//      std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) prefixes[i *
//prefixLen + j];
//    std::cout << ": " << std::dec << (size_t) counts[i] << "\n";
//  }

  alloc_hugepage<IndexType>(children, childrenMemState, numTotalNodes,
      "trie building");
  alloc_hugepage<uint8_t>(numChildren, numChildrenMemState, numTotalNodes,
      "trie building");
  alloc_hugepage<uint8_t>(bytes, bytesMemState, numTotalNodes,
      "trie building");
  alloc_hugepage<IndexType>(childrenVector, childrenVectorMemState,
      childrenVectorLen, "trie building");

  bytes[0] = 0; // does not matter for the root node
  children[0] = 0; // the root node's children start at the beginning of childrenVector
  size_t leafIndex = 0;
  result = BuildTrie(prefixes, numPrefixes, prefixLen, size_t(-1), 0, 1, 0, leafIndex);
  const size_t numNodesCreated = std::get<0>(result);
  const size_t numChildrenVecUsed = std::get<1>(result);
  std::cout << "supposed to be " << numTotalNodes << ", got " << numNodesCreated
<< "; children vector " << childrenVectorLen << " got " << numChildrenVecUsed
<< "\n";
  if (leafIndex > numPrefixes)
    throw std::runtime_error("invalid leaf index!");
  if (numNodesCreated != numTotalNodes)
    throw std::runtime_error("wrong number of nodes!");
  if (numChildrenVecUsed != childrenVectorLen)
    throw std::runtime_error("wrong children vector usage!");

//  size_t j = 0;
//  size_t i = 0;
//  uint8_t* triePrefix = new uint8_t[PrefixLen()];
//  memset(triePrefix, 0x00, sizeof(uint8_t) * PrefixLen());
//  CheckConsistency(prefixes, i, j, triePrefix, 0, 0);
//  delete[] triePrefix;
}

template<typename IndexType>
void PackedByteTrie<IndexType>::CheckConsistency(
    uint8_t* prefixes,
    size_t& leafIndex,
    size_t& j,
    uint8_t* triePrefix,
    const size_t nodeIndex,
    const size_t depth) const
{
  // No need to set a prefix byte at the root.
  if (depth > 0)
  {
    triePrefix[depth - 1] = bytes[nodeIndex];
    std::cout << "iterating, leaf index " << leafIndex << ", depth " << depth <<
", prefix trie bytes for node " << nodeIndex << " is "
        << std::hex << std::setw(2) << std::setfill('0') << (size_t) bytes[nodeIndex]
        << "; trie prefix now 0x";
    for (size_t i = 0; i < prefixLen; ++i)
      std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t)
triePrefix[i];
    std::cout << std::dec << "\n";
  }

  if (depth == prefixLen)
  {
    // This is a leaf.  Check that we have the right prefix.
    std::cout << "got prefix: 0x";
    for (size_t b = 0; b < prefixLen; ++b)
      std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) triePrefix[b];
    std::cout << "\n";
    std::cout << "expected prefix: 0x";
    for (size_t b = 0; b < prefixLen; ++b)
      std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) prefixes[leafIndex * prefixLen + b];
    std::cout << "\n";

    for (size_t b = 0; b < prefixLen; ++b)
      if (triePrefix[b] != prefixes[leafIndex * prefixLen + b])
        throw std::runtime_error("prefix is wrong");

    ++leafIndex;
  }
  else
  {
    const size_t nc = (numChildren[nodeIndex] == minChildrenForChildMap) ? 256 :
        (size_t) numChildren[nodeIndex];
    std::cout << "node has " << nc << " children\n";

    // We need to iterate over the children in sorted order.
    std::tuple<IndexType, uint8_t, uint8_t> childPairs[256];
    memset(childPairs, 0x00, sizeof(std::pair<IndexType, uint8_t>) * 256);
    size_t cFound = 0;
    for (size_t c = 0; c < nc; ++c)
    {
      if (childrenVector[children[nodeIndex] + c] == 0)
        childPairs[c] = std::make_tuple(std::numeric_limits<IndexType>::max(), c, 0);
      else
        childPairs[c] = std::make_tuple(children[childrenVector[children[nodeIndex] + c]], c, cFound++);
    }
    std::stable_sort(childPairs, childPairs + nc);

    for (size_t c = 0; c < nc; ++c)
    {
      const uint8_t cInd = std::get<1>(childPairs[c]);
      const uint8_t cLoc = std::get<2>(childPairs[c]);
      const size_t childNodeIndex =
          childrenVector[children[nodeIndex] + cInd];

        std::cout << "child " << std::dec << c << " (" << (size_t) cInd << ", loc " << (size_t) cLoc << ") at depth " << depth << " has index " <<
childNodeIndex << " (from childrenVector " << (children[nodeIndex] + cInd) << "\n";
      if (childNodeIndex != 0)
      {
        CheckConsistency(prefixes, leafIndex, j, triePrefix, childNodeIndex, depth + 1);
      }
    }
  }
}

template<typename IndexType>
PackedByteTrie<IndexType>::~PackedByteTrie()
{
  free_hugepage<IndexType>(children, childrenMemState, numTotalNodes);
  free_hugepage<uint8_t>(numChildren, numChildrenMemState, numTotalNodes);
  free_hugepage<uint8_t>(bytes, bytesMemState, numTotalNodes);
  free_hugepage<IndexType>(childrenVector, childrenVectorMemState,
      childrenVectorLen);
}

template<typename IndexType>
size_t PackedByteTrie<IndexType>::Search(const uint8_t* searchBytes) const
{
//  bool verbose = false;
//  if (prefixLen == 3 && searchBytes[0] == 0 && searchBytes[1] == 117 &&
//searchBytes[2] == 0)
//    verbose = true;

  //if (verbose)
  //{
  //std::cout << "search for 0x";
  //for (size_t i = 0; i < prefixLen; ++i)
  //  std::cout << std::hex << std::setw(2) << std::setfill('0') << (size_t) searchBytes[i];
  //std::cout << std::dec << "\n";
  //}
  // Start at the root.
  IndexType currentIndex = 0;
  for (size_t b = 0; b < prefixLen; ++b)
  {
    //if (verbose)
    //  std::cout << "check byte " << b << ": " << (size_t) searchBytes[b] << ", currentIndex " << currentIndex << "\n";
    // Search the current node's children.
    const size_t childCount = numChildren[currentIndex];
    const IndexType* currentChildren = childrenVector + children[currentIndex];
    if (childCount == 1)
    {
      //if (verbose)
      //  std::cout << "match on only child\n";
      if (searchBytes[b] == bytes[currentChildren[0]])
        currentIndex = currentChildren[0];
      else
      {
        //if (verbose)
        //  std::cout << "only child did not match\n";
        return size_t(-1);
      }
    }
    //else if (childCount == 2)
    //{
    //  if (searchBytes[b] == bytes[currentChildren[0]])
    //    currentIndex = currentChildren[0];
    //  else if (searchBytes[b] == bytes[currentChildren[0] + 1])
    //    currentIndex = currentChildren[0] + 1;
    //  else
    //    return size_t(-1);
    //}
    //else if (childCount >= 2 && childCount < minChildrenForChildMap)
    //{
    //  //if (verbose)
    //  //  std::cout << "currentIndex children: " << children[currentIndex] << "\n";
    //  // Fast binary search adapted from https://mhdm.dev/posts/sb_lower_bound/.
    //  size_t first = currentChildren[0];
    //  size_t length = childCount;
    //  while (length > 0)
    //  {
    //    //if (verbose)
    //    //  std::cout << "binary search b " << b << ", currentIndex " << currentIndex << " first " << first << " length " << length << "\n";
    //    size_t rem = length % 2;
    //    length /= 2;
    //    //if (verbose)
    //    //{
    //    //  std::cout << "check child " << (first + length);
    //    //  std::cout << " which has byte " << (size_t) bytes[first + length] << "\n";
    //    //  if (first + length > numTotalNodes)
    //    //    std::cout << "invalid access!!\n";
    //    //}
    //    if (bytes[first + length] < searchBytes[b])
    //      first += length + rem;
    //  }

    //  //if (verbose)
    //  //  std::cout << "final result: first " << first << "\n";
    //  if (first < (currentChildren[0] + childCount) && bytes[first] == searchBytes[b])
    //  {
    //    // Found the child.
    //    currentIndex = first;
    //  }
    //  else
    //  {
    //    // We don't have this child.
    //    return size_t(-1);
    //  }
    //}
    else if (childCount >= minChildrenForChildMap)
    {
      const IndexType& result = currentChildren[searchBytes[b]];
      if (result > 0)
      {
        //if (verbose)
        //  std::cout << "moving to child " << result << "\n";
        currentIndex = result;
      }
      else
        return size_t(-1);
    }
    else
    {
      return size_t(-1);
    }
  }

  //if (verbose)
  //{
  //  std::cout << "found child " << currentIndex << "\n";
  //  std::cout << "result is " << children[currentIndex] << "\n";
  //}
  return children[currentIndex];
}

template<typename IndexType>
std::tuple<size_t, size_t> PackedByteTrie<IndexType>::ReorderPrefixes(
    uint8_t* prefixes,
    uint32_t* counts,
    const size_t numPrefixes,
    const size_t prefixLen,
    const size_t byteIndex) const
{
  //std::cout << "reorder prefixes, numPrefixes " << numPrefixes << " byteIndex " << byteIndex << "\n";
  // We want to count the number of nodes that would be created by building this
  // subtree, and we want to reorder the prefixes.
  if (byteIndex == prefixLen - 1)
  {
    // We are only looking at leaves, and we will have `numPrefixes` of them.
    // However, we still need to sort them.
    //
    // This implementation is a little bit of a copout.  It could be optimized
    // by implementing an in-place sort on counts.
    std::pair<size_t, uint8_t> countPairs[256];
    for (size_t i = 0; i < 256; ++i)
      countPairs[i] = std::make_pair(0, i);
    for (size_t i = 0; i < numPrefixes; ++i)
    {
      const uint8_t byte = prefixes[i * prefixLen + byteIndex];
      countPairs[byte].first += counts[i];
    }

    // Now sort the prefixes by count.
    std::stable_sort(countPairs, countPairs + 256,
        [](const std::pair<size_t, uint8_t>& a,
           const std::pair<size_t, uint8_t>& b)
        {
          return a.first > b.first ||
              (a.first == b.first && a.second < b.second);
        });

    uint8_t* childPrefixes = new uint8_t[numPrefixes * prefixLen];
    uint32_t* childCounts = new uint32_t[numPrefixes];

    // Compute the location for each prefix.
    size_t prefixStarts[256];
    memset(prefixStarts, 0, sizeof(size_t) * 256);
    size_t currentOffset = 0;
    for (size_t c = 0; c < 256; ++c)
    {
      if (countPairs[c].first == 0)
        break;

      const uint8_t origIndex = countPairs[c].second;
      prefixStarts[origIndex] = currentOffset;
      ++currentOffset;
    }

    for (size_t i = 0; i < numPrefixes; ++i)
    {
      const uint8_t byte = prefixes[i * prefixLen + byteIndex];
      const size_t index = prefixStarts[byte];

      memcpy(&childPrefixes[index * prefixLen], &prefixes[i * prefixLen],
          prefixLen);
      childCounts[index] = counts[i];
    }

    memcpy(prefixes, childPrefixes, sizeof(uint8_t) * numPrefixes * prefixLen);
    memcpy(counts, childCounts, sizeof(uint32_t) * numPrefixes);
    delete[] childPrefixes;
    delete[] childCounts;

    return std::make_tuple(numPrefixes, numPrefixes >= minChildrenForChildMap ? 256 : numPrefixes);
  }
  else
  {
    std::bitset<256> hasBytes;
    std::pair<size_t, uint8_t> sumCounts[256];
    for (size_t i = 0; i < 256; ++i)
      sumCounts[i] = std::make_pair(0, i);
    size_t numChildPrefixes[256];
    memset(numChildPrefixes, 0, sizeof(size_t) * 256);
    for (size_t i = 0; i < numPrefixes; ++i)
    {
      const uint8_t byte = prefixes[i * prefixLen + byteIndex];
      hasBytes[byte] = true;
      sumCounts[byte].first += counts[i];
      ++numChildPrefixes[byte];
    }

    // Now sort the prefixes by count.
    std::stable_sort(sumCounts, sumCounts + 256,
        [](const std::pair<size_t, uint8_t>& a,
           const std::pair<size_t, uint8_t>& b)
        {
          return a.first > b.first ||
              (a.first == b.first && a.second < b.second);
        });

    // Compute the first location for each prefix.
    size_t prefixStarts[256];
    memset(prefixStarts, 0, sizeof(size_t) * 256);
    size_t currentOffset = 0;
    for (size_t c = 0; c < 256; ++c)
    {
      if (sumCounts[c].first == 0)
        break;

      const uint8_t origIndex = sumCounts[c].second;
      prefixStarts[origIndex] = currentOffset;
      currentOffset += numChildPrefixes[origIndex];
    }

    // Allocate memory for the next step.
    uint8_t* childPrefixes = new uint8_t[prefixLen * numPrefixes];
    uint32_t* childCounts = new uint32_t[numPrefixes];

    for (size_t i = 0; i < numPrefixes; ++i)
    {
      const uint8_t byte = prefixes[i * prefixLen + byteIndex];
      const size_t index = prefixStarts[byte];

      memcpy(&childPrefixes[index * prefixLen], &prefixes[i * prefixLen],
          prefixLen);
      childCounts[index] = counts[i];
      ++prefixStarts[byte];
    }

    // Overwrite the memory and deallocate.
    memcpy(prefixes, childPrefixes, sizeof(uint8_t) * prefixLen * numPrefixes);
    memcpy(counts, childCounts, sizeof(uint32_t) * numPrefixes);
    delete[] childPrefixes;
    delete[] childCounts;

    // If we will have a child map, we need to report that we are creating those
    // children for the children vector.
    size_t descendantCount = hasBytes.count();
    size_t childrenVectorCount = (hasBytes.count() >= minChildrenForChildMap) ?
        256 : hasBytes.count();
    for (size_t c = 0; c < 256; ++c)
    {
      if (sumCounts[c].first == 0)
        break; // We are sorted in descending order.

      const uint8_t origIndex = sumCounts[c].second;
      // Remember, we just incremented prefixStarts once for each prefix.
      const size_t startPrefix = prefixStarts[origIndex] -
          numChildPrefixes[origIndex];
      std::tuple<size_t, size_t> result = ReorderPrefixes(
          &prefixes[startPrefix * prefixLen], &counts[startPrefix],
          numChildPrefixes[origIndex], prefixLen, byteIndex + 1);
      descendantCount += std::get<0>(result);
      childrenVectorCount += std::get<1>(result);
    }

    return std::make_tuple(descendantCount, childrenVectorCount);
  }
}

template<typename IndexType>
std::tuple<size_t, size_t> PackedByteTrie<IndexType>::BuildTrie(
    const uint8_t* prefixes,
    const size_t numPrefixes,
    const size_t prefixLen,
    const size_t byteIndex,
    // position in bytes[] and children[]
    // of node we are building
    const size_t currentPos,
    const size_t childrenStartPos,
    const size_t currentChildrenVecPos,
    size_t& leafIndex)
{
  // Note that we should never call this on a leaf node.

  // Count the number of children this node will have.  Also count the
  // rank-ordering of each child.
  const size_t nextByte = byteIndex + 1;
  std::bitset<256> hasBytes;
  uint8_t byteOrder[256];
  size_t numChildPrefixes[256];
  memset(byteOrder, 0x00, sizeof(uint8_t) * 256);
  memset(numChildPrefixes, 0x00, sizeof(size_t) * 256);
  for (size_t i = 0; i < numPrefixes; ++i)
  {
    const uint8_t& byte = prefixes[i * prefixLen + nextByte];
    if (!hasBytes[byte])
      byteOrder[hasBytes.count()] = byte;
    hasBytes[byte] = true;
    ++numChildPrefixes[byte];
  }

  uint8_t childIndices[256];
  memset(childIndices, 0x00, sizeof(uint8_t) * 256);
  size_t c = 0;
  for (size_t i = 0; i < 256; ++i)
    if (hasBytes[i])
      childIndices[i] = c++;

  // Set the byte and number of children, and the position in the children
  // vector that the locations of our children will start at.
  bytes[currentPos] = (byteIndex == (size_t) -1) ? 0 : prefixes[byteIndex];
  numChildren[currentPos] = (hasBytes.count() >= minChildrenForChildMap) ?
      minChildrenForChildMap : hasBytes.count();
  children[currentPos] = currentChildrenVecPos;

//  std::cout << "create node: byte index " << byteIndex << ", currentPos " <<
//currentPos << ", byte 0x" << std::hex << std::setw(2) << std::setfill('0') <<
//(size_t) bytes[currentPos] << ", numChildren " << std::dec <<
//(size_t) numChildren[currentPos] << ", children " << (size_t) children[currentPos] << "\n";

  if (numChildren[currentPos] == minChildrenForChildMap)
  {
    // Initialize all children locations to 0.
    memset(childrenVector + currentChildrenVecPos, 0x00,
        sizeof(IndexType) * 256);
  }

  // The descendants will be laid out in a breadth-first manner.  Thus, the
  // child position is a small offset from currentPos, but the child vector
  // position will depend on the number of nodes that each child creates.
  // However, we want to iterate in priority-order of bytes.
  size_t prefixOffset = 0;
  size_t totalNodesCreated = 0;
  size_t totalChildrenVecUsed = 0;
  for (c = 0; c < hasBytes.count(); ++c)
  {
    const uint8_t& childByte = byteOrder[c];
    const uint8_t& childIndex = childIndices[childByte];
    // This is the location in the array for the new child's bytes, numChildren,
    // and children pointer.
    const size_t childLoc = childrenStartPos + childIndex;
    const size_t childChildrenStartLoc = childrenStartPos + hasBytes.count() +
        totalNodesCreated - c;
    size_t childChildrenVecPos = currentChildrenVecPos + hasBytes.count() +
        totalChildrenVecUsed;
    // The calculation for childChildrenVecPos is actually different if we have
    // a child map.
    if (numChildren[currentPos] == minChildrenForChildMap)
      childChildrenVecPos = currentChildrenVecPos + 256 + totalChildrenVecUsed;

    // Set link to the child we are about to create.  This points to the
    // bytes[], numChildren[], and children[] locations for the child node.
    if (numChildren[currentPos] == minChildrenForChildMap)
      childrenVector[currentChildrenVecPos + childByte] = childLoc;
    else
      childrenVector[currentChildrenVecPos + childIndex] = childLoc;

    if (byteIndex + 1 != prefixLen - 1)
    {
      const std::tuple<size_t, size_t> result = BuildTrie(
          prefixes + (prefixOffset * prefixLen), numChildPrefixes[childByte],
          prefixLen, byteIndex + 1, childLoc, childChildrenStartLoc,
          childChildrenVecPos, leafIndex);
      const size_t nodesCreated = std::get<0>(result);
      const size_t childrenVecUsed = std::get<1>(result);

      prefixOffset += numChildPrefixes[childByte];
      totalNodesCreated += nodesCreated;
      totalChildrenVecUsed += childrenVecUsed;
    }
    else
    {
      // Don't bother recursing to make a leaf.
      bytes[childLoc] = childByte;
      numChildren[childLoc] = 0;
      children[childLoc] = leafIndex++; // this is the value of the leaf
//      std::cout << "create leaf node at pos " << childLoc << " with byte 0x" << std::hex << std::setw(2) << std::setfill('0') << (size_t) childByte << " and value " << std::dec << (size_t) (leafIndex - 1) << "\n";
      ++totalNodesCreated;
      // NOTE: we didn't use an element in the children vector, so no need to
      // increment
    }
  }

  if (numChildren[currentPos] == minChildrenForChildMap)
    totalChildrenVecUsed += 256;
  else
    totalChildrenVecUsed += numChildren[currentPos];
  return std::make_tuple(totalNodesCreated + 1, totalChildrenVecUsed);
}

#endif
