// find_top_k.hpp: utility to find the top-k n-grams
#ifndef PNGRAM_FIND_TOP_K_HPP
#define PNGRAM_FIND_TOP_K_HPP

#include <map>

inline uint32_t ComputeCutoff(
    const std::map<uint32_t, uint32_t, std::greater<uint32_t>>& countMap,
    const size_t countTotal,
    const size_t cutoff)
{
  // Sanity check: if we didn't find enough n-grams, no need to do anything.
  if (countTotal < cutoff)
    return 1;

  // Pass through the ordered map to figure out where the top-k'th count is.
  // Our map's key is the n-gram counts, and the value is the number of times
  // that n-gram count was encountered (I know, it's a count count...
  // sorry...).  If we count up the count counts (...sorry again...), when that
  // sum gets to `k`, then the n-gram hash count (the first element of the map)
  // will be the k'th sorted n-gram hash.
  size_t sum = 0;
  std::map<uint32_t, uint32_t>::const_iterator it = countMap.begin();
  while (it != countMap.end() && sum <= cutoff)
  {
    sum += (*it).second;
    ++it;
  }

  if (it == countMap.end())
  {
    return 1;
  }
  else if (it != countMap.begin())
  {
    // We have advanced one past the bin of interest, so step back.
    return (*(--it)).first;
  }
  else
  {
    // We're looking at the first bin already, so we can't take a step back.
    return (*it).first;
  }
}

// prefixes must already be allocated
template<typename CountsArrayType>
size_t FindTopK(CountsArrayType& counts,
                const size_t prefixLen,
                const size_t k,
                uint8_t* prefixes,
                uint32_t* prefixCounts)
{
  // Extract nonzero n-grams where the count is greater than 1.
  // There's no need to collect n-grams where the count is only 1,
  // since we always get min_count = 1 or greater.
  std::map<uint32_t, uint32_t, std::greater<uint32_t>> countMap;
  size_t countTotal = 0;
  for (size_t i = 0; i < counts.Size(); ++i)
  {
    const uint32_t count = counts[i];
    if (count > 1)
    {
      //if (prefixLen > 3)
      //  std::cout << "prefix " << i << " (" << (i / 256) << ") has count " << count << "\n";
      ++countMap[count];
      ++countTotal;
    }
  }

  const size_t minCount = ComputeCutoff(countMap, countTotal, k);
  size_t usedBeforeLastBin = 0;
  std::map<uint32_t, uint32_t, std::greater<uint32_t>>::const_iterator it =
      countMap.begin();
  while (it != countMap.end() && it->first > minCount)
  {
    usedBeforeLastBin += it->second;
    ++it;
  }

  const size_t lastBinLimit = k - usedBeforeLastBin;
  return counts.CopyPrefixes(prefixes, minCount, lastBinLimit, prefixCounts);
}

#endif
