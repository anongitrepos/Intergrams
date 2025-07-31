#ifndef PNGRAM_GRAM_DATA_IMPL_HPP
#define PNGRAM_GRAM_DATA_IMPL_HPP

#include "gram_data.hpp"

inline GramData::GramData() :
    flushStatus1(0),
    flushStatus2(0),
    fileId1(0),
    fileId2(0)
{
  memset(bits1, 0, sizeof(uint8_t) * TotalByteSize);
  memset(bits2, 0, sizeof(uint8_t) * TotalByteSize);

  alloc_hugepage<std::atomic_uint32_t>(gram3Counts, gram3CountsMemState,
      TotalAddressCount, "n-gramming");
  memset(gram3Counts, 0, sizeof(uint32_t) * TotalAddressCount);
}

inline GramData::~GramData()
{
  free_hugepage<std::atomic_uint32_t>(gram3Counts, gram3CountsMemState,
      TotalAddressCount);
}

#endif
