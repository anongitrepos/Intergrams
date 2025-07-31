#ifndef PNGRAM_DIRECTORY_ITERATOR_HPP
#define PNGRAM_DIRECTORY_ITERATOR_HPP

#include <filesystem>
#include <vector>
#include <mutex>

namespace fs = std::filesystem;

// An iterator that returns files in a threadsafe manner.
struct DirectoryIterator
{
 public:
  DirectoryIterator(const std::vector<fs::path>& paths_to_explore, const bool count_files);

  size_t get_current_file() const { return current_file; }
  size_t get_file_count() const { return file_count; }

  inline bool get_next(fs::path& result, size_t& file_index);

  inline void reset();

 private:
  inline void step();

  std::vector<fs::path> paths;
  size_t path_index;

  size_t file_count;
  size_t current_file;

  fs::recursive_directory_iterator it;
  std::mutex it_mutex;

  // Locally-cached next file, needed because we step at the end of get_next()
  fs::path local_result;
};

#include "directory_iterator_impl.hpp"

#endif // PNGRAM_DIRECTORY_ITERATOR_HPP
