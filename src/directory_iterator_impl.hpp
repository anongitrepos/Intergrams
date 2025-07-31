#ifndef PNGRAM_DIRECTORY_ITERATOR_IMPL_HPP
#define PNGRAM_DIRECTORY_ITERATOR_IMPL_HPP

#include "directory_iterator.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

#define MAX_DEPTH 10

inline DirectoryIterator::DirectoryIterator(
    const std::vector<fs::path>& paths_to_explore,
    const bool count_files) :
    paths(paths_to_explore),
    path_index(0),
    file_count(0),
    current_file(size_t(-1)) /* so that we will wrap over to 0 on the first step */
{
  // Count files, if needed.
  if (count_files)
  {
    for (const fs::path& path : this->paths)
    {
      if (fs::is_regular_file(path))
      {
        ++this->file_count;
      }
      else if (fs::is_symlink(path) &&
               fs::is_regular_file(fs::read_symlink(path)))
      {
        ++this->file_count;
      }
      else
      {
        fs::recursive_directory_iterator p_it(path,
            fs::directory_options::follow_directory_symlink);

        for (const fs::directory_entry& entry : p_it)
        {
          if (entry.is_regular_file())
            ++this->file_count;
          else if (entry.is_symlink() &&
                   fs::is_regular_file(fs::read_symlink(entry.path())))
            ++this->file_count;

          // If this happens, we will break during the actual iteration, so no
          // warning here.
          if (p_it.depth() > MAX_DEPTH)
              break;
        }
      }
    }
  }

  // Create the first iterator, if we can.
  if (this->paths.size() > 0)
  {
    if (fs::is_regular_file(this->paths[0]))
    {
      this->local_result = this->paths[0];
      // Use an end iterator so we advance to the next path immediately.
      this->it = fs::recursive_directory_iterator();
    }
    else if (fs::is_symlink(this->paths[0]) &&
             fs::is_regular_file(fs::read_symlink(this->paths[0])))
    {
      this->local_result = fs::read_symlink(this->paths[0]);
      // Use an end iterator so we advance to the next path immediately.
      this->it = fs::recursive_directory_iterator();
    }
    else
    {
      this->it = fs::recursive_directory_iterator(this->paths[0],
          fs::directory_options::follow_directory_symlink);
      // Make sure we are looking at a file.
      step();
    }
  }
  else
  {
    // Initialize the iterator to an end iterator.
    this->it = fs::recursive_directory_iterator();
    this->current_file = 0;
    this->file_count = 0;
  }
}

inline bool DirectoryIterator::get_next(fs::path& result, size_t& file_index)
{
  this->it_mutex.lock();
  if (this->local_result.empty())
  {
    // No more entries---return false.
    file_index = 0;
    this->current_file = this->file_count;
    this->it_mutex.unlock();
    return false;
  }

  result = this->local_result;
  this->local_result.clear();

  step();
  file_index = ++this->current_file;

  this->it_mutex.unlock();
  return true;
}

inline void DirectoryIterator::step()
{
  // Advance the iterator to point at the next file.  We might have to iterate
  // multiple times to get something that we consider a file.
  this->local_result.clear();
  while (this->local_result.empty() && this->it != fs::recursive_directory_iterator())
  {
    const fs::directory_entry& entry = (*this->it);
    if (entry.is_regular_file())
    {
      this->local_result = entry.path();
    }
    else if (entry.is_symlink())
    {
      fs::path symPath = fs::read_symlink(entry.path());
      if (fs::is_regular_file(symPath))
        this->local_result = symPath;
    }

    ++this->it;
    if (this->it != fs::recursive_directory_iterator() &&
        this->it.depth() > MAX_DEPTH)
    {
      std::cerr << "Warning: Load file path reached the max depth of "
          << MAX_DEPTH << ", not visiting any more files" << std::endl;
      this->it = fs::recursive_directory_iterator(); // set to end iterator
      break;
    }
  }

  // Did we get to the end of this recursion and need to look at another
  // directory or file?
  if (this->it == fs::recursive_directory_iterator() &&
      this->path_index + 1 < this->paths.size())
  {
    this->path_index++;
    if (fs::is_regular_file(this->paths[this->path_index]))
    {
      // Don't change the iterator---it should still be an end iterator.
      this->local_result = this->paths[this->path_index];
    }
    else if (fs::is_symlink(this->paths[this->path_index]) &&
        fs::is_regular_file(fs::read_symlink(this->paths[this->path_index])))
    {
      // Don't change the iterator---it should still be an end iterator.
      this->local_result = fs::read_symlink(this->paths[this->path_index]);
    }
    else
    {
      this->it = fs::recursive_directory_iterator(this->paths[this->path_index],
          fs::directory_options::follow_directory_symlink);
    }

    // And do we need to search for a file?
    if (this->local_result.empty())
    {
      // Just call step() again; this will advance us to a file (or to the next
      // path if needed), until we run out of files and paths.
      step();
    }
  }
}

inline void DirectoryIterator::reset()
{
  this->current_file = size_t(-1);
  this->path_index = 0;
  if (this->paths.size() > 0)
  {
    if (fs::is_regular_file(this->paths[0]))
    {
      // Use an invalid (end) iterator.
      this->it = fs::recursive_directory_iterator();
      this->local_result = this->paths[0];
    }
    else if (fs::is_symlink(this->paths[0]) &&
        fs::is_regular_file(fs::read_symlink(this->paths[0])))
    {
      // Use an invalid (end) iterator.
      this->it = fs::recursive_directory_iterator();
      this->local_result = fs::read_symlink(this->paths[0]);
    }
    else
    {
      this->it = fs::recursive_directory_iterator(this->paths[0],
          fs::directory_options::follow_directory_symlink);
      step();
    }
  }
  else
  {
    // There are no paths, so set the iterator to an end iterator.
    this->it = fs::recursive_directory_iterator();
    this->current_file = 0;
  }
}

#endif
