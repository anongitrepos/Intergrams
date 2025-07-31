// alloc.hpp: utilities for allocating memory with large pages on different
// systems
#ifndef PNGRAM_ALLOC_HPP
#define PNGRAM_ALLOC_HPP

#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
    #include <sys/mman.h>
    #include <mach/vm_statistics.h>
#elif __linux__
    #include <sys/mman.h>
#endif
#include <string>

// These are the four strategies we may use when allocating memory.
enum struct alloc_mem_state
{
  mem_state_new,
  mem_state_malloc,
  mem_state_posix_memalign,
  mem_state_mmap
};

// Attempt to allocate a block of memory using huge pages (typically 2MB).
// Throws an exception on allocation failure.
//
//  - `ptr` will be set to the location of the allocated memory.
//  - `mem_state` will be set to a different value depending on the allocation
//          strategy used; pass this to `free_hugepage()` when deallocating.
//  - `count` is the number of `T`s to allocate (that is, the allocated buffer
//          will have size `(sizeof(T) * count)`.
//  - `purpose` is used to generate an exception error message, e.g., "could not
//          allocate memory for <purpose>".
//
// If `mem_state` is not `0`, or if the `new` operator will not initialize a
// `T`, then memory has not been initialized after allocation---that is up to
// you (use memset(), for instance).
template<typename T>
void alloc_hugepage(T*& ptr,
                    alloc_mem_state& mem_state,
                    size_t count,
                    const char* purpose)
{
  #if __APPLE__
    // First, try to allocate the array with 2MB huge pages, if possible.  Note
    // that this may not work on ARM (M1, M2, etc.).  The OS X documentation is
    // pretty poor on how to do this correctly.
    ptr = (T*) mmap(NULL, sizeof(T) * count, PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_PRIVATE, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
    mem_state = alloc_mem_state::mem_state_mmap;

    if (ptr == (T*) -1)
    {
      // We use C-style malloc here because we want to avoid the initializer
      // being called; on OS X, this can be very slow for std::atomic types
      // (e.g., it can take 15 seconds!).
      ptr = (T*) malloc(sizeof(T) * count);
      mem_state = alloc_mem_state::mem_state_malloc;
    }

    if (!ptr)
    {
      throw std::runtime_error(std::string("could not allocate memory for ") +
          std::string(purpose) + std::string(": ") + strerror(errno));
    }
  #elif __linux__
    // Attempt to allocate so that we get transparent huge pages of size 2MB.
    int alloc_success = posix_memalign((void**) &ptr, (1 << 21),
        sizeof(T) * count);
    if (alloc_success == 0)
    {
      // We ignore if madvise() returned an error, since we still succeeded at
      // allocating the memory.
      madvise(ptr, sizeof(T) * count, MADV_HUGEPAGE);
      mem_state = alloc_mem_state::mem_state_posix_memalign;
    }
    else
    {
      // Fall back to regular allocation.
      ptr = new T[count];
      mem_state = alloc_mem_state::mem_state_new;
    }
  #else
    // Unknown platform---just allocate in the usual way with new.
    ptr = new T[count];
    mem_state = alloc_mem_state::mem_state_new;
  #endif
}

template<typename T>
void free_hugepage(T*& ptr, alloc_mem_state mem_state, size_t count)
{
  if (ptr == NULL || count == 0)
    return; // Nothing to do.

  if (mem_state == alloc_mem_state::mem_state_new)
  {
    delete[] ptr;
  }
  else if (mem_state == alloc_mem_state::mem_state_malloc)
  {
    free(ptr);
  }
  else if (mem_state == alloc_mem_state::mem_state_posix_memalign)
  {
    free(ptr);
  }
  else if (mem_state == alloc_mem_state::mem_state_mmap)
  {
    // We don't have munmap() on other platforms.
    #if defined(__APPLE__) || defined(__linux__)
      munmap(ptr, sizeof(T) * count);
    #endif
  }
}

#endif
