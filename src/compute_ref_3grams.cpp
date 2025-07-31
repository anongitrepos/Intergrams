#include <bitset>
#include "directory_iterator.hpp"
#include <fstream>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <directory> <outfile>" << std::endl;
    exit(1);
  }

  const std::string directory(argv[1]);
  const std::string outfile(argv[2]);
  DirectoryIterator dIter({ directory }, false);
  std::bitset<16777216> bits;
  uint32_t* counts = new uint32_t[16777216];
  memset(counts, 0, sizeof(uint32_t) * 16777216);

  unsigned char local_buffer[4096];
  size_t chunk_size = 4096;

  std::filesystem::path p;
  size_t i;
  while (dIter.get_next(p, i))
  {
    if (i % 100 == 0)
      std::cout << "Processing file " << i << "..." << std::endl;

    int fd = open(p.c_str(), O_RDONLY); // TODO: O_DIRECT
    if (fd == -1)
    {
      // check errno, something went wrong
      std::cout << "open failed! errno " << errno << "\n";
      continue;
    }

    // read into buffer
    ssize_t bytes_read = 0;
    do
    {
      bytes_read = read(fd, local_buffer, chunk_size);
      if (bytes_read > 2)
      {
        // process chunk
        for (size_t b = 0; b < (bytes_read - 2); ++b)
        {
          const size_t index = ((size_t(local_buffer[b]) << 16) +
              (size_t(local_buffer[b + 1]) << 8) + size_t(local_buffer[b + 2]));
          bits[index] = true;
        }

        // rewind for next read
        off_t o = -2;
        if (lseek(fd, -o, SEEK_CUR) == -1)
          std::cout << "lseek fail!\n";
      }
      else if (bytes_read == -1)
      {
        // error
        std::cout << "read failed! errno " << errno << "\n";
      }

    } while (bytes_read > 0);

    close(fd);

    // Flush the bits.
    for (size_t i = 0; i < 16777216; ++i)
      if (bits[i])
        ++counts[i];

    bits.reset();
  }
  std::cout << "Now flushing\n";

  std::fstream f(outfile, std::fstream::out);
  for (size_t i = 0; i < 16777216; ++i)
  {
    f << "0x" << std::hex << std::setw(6) << std::setfill('0') << i << ", "
        << std::dec << counts[i] << "\n";
  }

  delete[] counts;
}
