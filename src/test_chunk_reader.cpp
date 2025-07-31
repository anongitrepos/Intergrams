#include "reader_thread.hpp"
#include <armadillo>

class ChunkReader
{
 public:
  ChunkReader(ReaderThread& reader) :
      reader(reader),
      count(0),
      sum(0),
      thread(&ChunkReader::RunThread, this)
  {
    // Nothing to do.
  }

  void RunThread()
  {
    unsigned char* ptr = nullptr;
    size_t bytes = -1;
    size_t file_id = -1;
    reader.get_next_chunk(ptr, bytes, file_id);
    if (ptr == NULL)
    {
      std::cerr << "First chunk was null!" << std::endl;
      return;
    }

    do
    {
      count += bytes;
      #pragma unroll
      for (size_t i = 0; i < bytes; ++i)
        sum += ptr[i];
      reader.finish_chunk(ptr, file_id);

      // check if we need to 'flush' a file
      reader.get_next_flushee(file_id);
      if (file_id != size_t(-1))
        reader.finish_flush(file_id);

      bool done = false;
      do
      {
        done = !reader.get_next_chunk(ptr, bytes, file_id);
        if (done)
          break;

        if (ptr == NULL)
          usleep(100);
      } while (ptr == NULL);

      if (done)
        break;
    } while (true);
  }

  void Finish()
  {
    if (thread.joinable())
      thread.join();
  }

  size_t Count() const { return count; }
  size_t Sum() const { return sum; }

 private:
  ReaderThread& reader;
  size_t count;
  size_t sum;
  std::thread thread;
};

int main(int argc, char** argv)
{
  // The goal is to create a ThreadReader and another thread that will
  // actually process the chunks; the chunk processor is just going to
  // truncated-sum the bytes so it can pass over them once.  Then we'll time
  // the whole thing to compute the bandwidth.
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " directory/ <threads>" << std::endl;
    exit(1);
  }

  arma::wall_clock c;

  c.tic();
  // This will start getting chunks immediately.
  std::filesystem::path d(argv[1]);
  const size_t threads = atoi(argv[2]);
  DirectoryIterator d_it({ d }, false);

  // First create all reader threads.
  ReaderThread** readerThreads = new ReaderThread*[threads];
  for (size_t t = 0; t < threads; ++t)
    readerThreads[t] = new ReaderThread(d_it, 3);

  // Let them all initialize...
  usleep(1000);

  // Now create all chunk reader threads.
  ChunkReader** chunkThreads = new ChunkReader*[threads];
  for (size_t t = 0; t < threads; ++t)
    chunkThreads[t] = new ChunkReader(*readerThreads[t]);

  // Let them all initialize...
  usleep(1000);

  // Wait until all the work is done...
  for (size_t t = 0; t < threads; ++t)
    chunkThreads[t]->Finish();

  for (size_t t = 0; t < threads; ++t)
    delete readerThreads[t];

  std::cout << "Everything took " << c.toc() << "s." << std::endl;
  std::cout << "Thread statistics:" << std::endl;
  for (size_t t = 0; t < threads; ++t)
  {
    std::cout << " - Thread " << t << ": " << chunkThreads[t]->Count()
        << " bytes, " << chunkThreads[t]->Sum() << " sum." << std::endl;
  }

  for (size_t t = 0; t < threads; ++t)
    delete chunkThreads[t];

  delete[] readerThreads;
  delete[] chunkThreads;
}
