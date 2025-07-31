// reader_thread_impl.hpp: implementation of ReaderThread functionality.
#ifndef PNGRAM_READER_THREAD_IMPL_HPP
#define PNGRAM_READER_THREAD_IMPL_HPP

#include "reader_thread.hpp"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

inline ReaderThread::ReaderThread(DirectoryIterator& d_it_in,
                                  const size_t n) :
    d_it(d_it_in),
    local_buffer(new unsigned char[total_buffer_size]),
    chunk_sizes(new size_t[num_chunks]),
    next_available_chunk(0),
    next_unused_chunk(0),
    n(n),
    waiting_for_files(0),
    waiting_for_chunks(0),
    thread(&ReaderThread::thread_run, this),
    finished(false)
{
  // The thread has now started.
}

inline ReaderThread::~ReaderThread()
{
  thread.join();

  bool done = false;
  while (!done)
  {
    bool any_left = false;
    for (size_t i = 0; i < num_chunks; ++i)
    {
      if (chunk_ready[i] || chunk_claimed[i])
      {
        any_left = true;
        break;
      }
    }

    if (!any_left)
    {
      for (size_t i = 0; i < num_files; ++i)
      {
        if (file_chunks_left[i] > 0)
        {
          any_left = true;
          break;
        }
      }
    }

    if (!any_left)
    {
      done = true;
      break;
    }

    usleep(1000);
  }

  std::ostringstream oss;
  oss << "ReaderThread statistics:\n";
  oss << " - " << waiting_for_files << " iterations waiting on a file to be open.\n";
  oss << " - " << waiting_for_chunks << " iterations waiting on a chunk to be available.\n";
  std::cout << oss.str();

  delete[] local_buffer;
  delete[] chunk_sizes;
}

inline void ReaderThread::thread_run()
{
  // mark all chunks unclaimed and unavailable
  for (size_t i = 0; i < num_chunks; ++i)
  {
    chunk_ready[i] = false;
    chunk_claimed[i] = false;
  }

  for (size_t i = 0; i < num_files; ++i)
    file_chunks_left[i] = 0;

  std::filesystem::path p;
  size_t i;
  size_t current_file_index;
  while (d_it.get_next(p, i))
  {
    if (i % 10000 == 0)
      std::cout << "Reading file " << i << "..." << std::endl;

    bool found_file_index = false;
    while (!found_file_index)
    {
      for (current_file_index = 0; current_file_index < num_files;
           ++current_file_index)
      {
        if (file_chunks_left[current_file_index] == 0)
        {
          found_file_index = true;
          break;
        }
      }

      // Maybe chunks have not been processed... wait for a little while.
      if (!found_file_index)
      {
        ++waiting_for_files;
        usleep(1000);
      }
    }

    file_chunks_left[current_file_index] = 3;

    int fd = open(p.c_str(), O_RDONLY); // TODO: O_DIRECT
    if (fd == -1)
    {
      // check errno, something went wrong
      std::cout << "open failed! errno " << errno << "\n";
      file_chunks_left[current_file_index] = 0;
      continue;
    }

    // read into buffer
    ssize_t bytes_read = 0;
    do
    {
      // Find the chunk we will read into.  There are cases in which we will
      // need to wait...
      size_t target_chunk = next_unused_chunk;
      while (true)
      {
        do
        {
          if (!chunk_ready[target_chunk] && !chunk_claimed[target_chunk])
            break;
          target_chunk = (target_chunk + 1) % num_chunks;
        } while (target_chunk != next_unused_chunk);

        // Did we find a chunk at all?
        if (!chunk_ready[target_chunk] && !chunk_claimed[target_chunk])
          break;

        // We didn't find a chunk, so we need to wait...
        ++waiting_for_chunks;
        usleep(1000);
      }

      bytes_read = read(fd, local_buffer + target_chunk * chunk_size,
          chunk_size);
      if (bytes_read > 0)
      {
        // advance to next chunk
        ++file_chunks_left[current_file_index];
        chunk_file_ids[target_chunk] = current_file_index;
        chunk_sizes[target_chunk] = bytes_read;
        chunk_ready[target_chunk] = true;
        next_available_chunk = target_chunk;

        // rewind for next read
        off_t o = -(n - 1);
        if (lseek(fd, -o, SEEK_CUR) == -1)
          std::cout << "lseek fail!\n";
      }
      else if (bytes_read == -1)
      {
        // error
        std::cout << "read failed! errno " << errno << "\n";
      }
    } while (bytes_read > 0);

    // since we are done with the file, remove the extra chunk count
    // if we are done with the file, mark it as complete
    if (--file_chunks_left[current_file_index] == 2)
      ++needs_flush_count;

    close(fd);
  }

  finished = true;
}

inline bool ReaderThread::get_next_chunk(unsigned char*& ptr,
                                         size_t& bytes,
                                         size_t& file_id)
{
  // Search for a free chunk.  The mutex ensures only one thread is trying to
  // find a chunk at a time (the reader thread itself may be preparing chunks
  // simultaneously but that is okay).
  next_chunk_mutex.lock();
  size_t target_chunk = next_available_chunk;
  do
  {
    if (chunk_ready[target_chunk] && !chunk_claimed[target_chunk])
      break;
    target_chunk = (target_chunk + 1) % num_chunks;
  } while (target_chunk != next_available_chunk);

  // Did we find a chunk at all?
  if (chunk_ready[target_chunk] && !chunk_claimed[target_chunk])
  {
    chunk_claimed[target_chunk] = true;
    next_chunk_mutex.unlock();
    ptr = local_buffer + target_chunk * chunk_size;
    bytes = chunk_sizes[target_chunk];
    file_id = chunk_file_ids[target_chunk];
    return true;
  }

  // We didn't find a chunk.
  next_chunk_mutex.unlock();
  ptr = NULL;
  bytes = 0;
  file_id = size_t(-1);
  return !finished;
}

// 0: file is done
// 1: waiting on flush
// 2: no more chunks available, ready for flush
// 3: more chunks available, but maybe not yet

inline void ReaderThread::finish_chunk(const unsigned char* ptr, const size_t file_id)
{
  const size_t chunk_id = (ptr - local_buffer) / chunk_size;
  next_chunk_mutex.lock();
  chunk_ready[chunk_id] = false;
  --file_chunks_left[file_id];
  chunk_claimed[chunk_id] = false;

  next_unused_chunk = chunk_id;
  next_chunk_mutex.unlock();
}

inline bool ReaderThread::get_next_flushee(size_t& index)
{
  bool has_any = false;
  for (size_t i = 0; i < num_files; ++i)
  {
    uint32_t chunks_left = 2;
    if (file_chunks_left[i].compare_exchange_strong(chunks_left, 1))
    {
      index = i;
      return true;
    }
    else if (chunks_left > 2)
    {
      has_any = true;
    }
  }

  index = -1;
  return !finished || has_any;
}

inline void ReaderThread::finish_flush(const size_t file_id)
{
  if (file_chunks_left[file_id] != 1) {
    std::ostringstream oss;
    oss << "invalid finish flush file_id " << file_id << " with chunks left " << file_chunks_left[file_id] << "\n";
    throw std::invalid_argument(oss.str());
  }
  --needs_flush_count;
  --file_chunks_left[file_id];
}

#endif
