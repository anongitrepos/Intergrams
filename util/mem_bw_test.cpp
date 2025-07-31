/**
 * A simple test of random access and sequential access patterns based on memory
 * size.
 *
 * Outputs a csv to the given file.
 */
#include <iostream>
#include <stdint.h>
#include <armadillo>
#include <chrono>

int main(int argc, char** argv)
{
  if (argc < 2)
    std::cout << "Outputting to 'mem_bw.csv'..." << std::endl;
  else
    std::cout << "Outputting to '" << argv[1] << "'..." << std::endl;

  std::string outFile = (argc < 2) ? "mem_bw.csv" : argv[1];
  std::fstream of(outFile, std::fstream::out);
  of << "power, bytes, seqbw, seqhibw, seqlobw, rndbw, rndhibw, rndlobw" << std::endl;

  for (size_t p = 4; p <= 34; ++p)
  {
    const size_t elem = std::pow(2, p);

    arma::Col<uint32_t> a(elem, arma::fill::zeros);
    if (p <= 16)
    {
      // Run 5 warmup trials.
      for (size_t trial = 0; trial < 5; ++trial)
        a.zeros();
    }

    const size_t trials = (p <= 16) ? 100 :
        (p <= 24) ? 20 : 5;

    typedef std::chrono::high_resolution_clock ClockType;
    typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimeType;
    arma::vec seqTimes(trials);
    for (size_t trial = 0; trial < trials; ++trial)
    {
      TimeType t1 = ClockType::now();
      for (size_t i = 0; i < a.n_elem; i++)
        a[i] += i + 1;
      TimeType t2 = ClockType::now();
      std::chrono::duration<double> d = t2 - t1;
      seqTimes[trial] = d.count();
    }

    arma::Col<size_t> order(elem);
    arma::vec rndTimes(trials);
    for (size_t trial = 0; trial < trials; ++trial)
    {
      order = arma::shuffle(arma::linspace<arma::Col<size_t>>(0, elem - 1, elem));

      TimeType t1 = ClockType::now();
      for (size_t i = 0; i < a.n_elem; i++)
        a[order[i]] += i + 1;
      TimeType t2 = ClockType::now();
      std::chrono::duration<double> d = t2 - t1;
      rndTimes[trial] = d.count();
    }

    const double seqTime = arma::mean(seqTimes);
    const double rndTime = arma::mean(rndTimes);

    const double seqLowerTime = seqTime - arma::stddev(seqTimes);
    const double seqUpperTime = seqTime + arma::stddev(seqTimes);

    const double rndLowerTime = rndTime - arma::stddev(rndTimes);
    const double rndUpperTime = rndTime + arma::stddev(rndTimes);

    const size_t bytes = sizeof(uint32_t) * elem;
    const double seqBandwidth = (bytes / std::pow(2, 30)) / seqTime;
    const double seqLoBandwidth = (bytes / std::pow(2, 30)) / seqUpperTime;
    const double seqHiBandwidth = (bytes / std::pow(2, 30)) / seqLowerTime;
    const double rndBandwidth = (bytes / std::pow(2, 30)) / rndTime;
    const double rndLoBandwidth = (bytes / std::pow(2, 30)) / rndUpperTime;
    const double rndHiBandwidth = (bytes / std::pow(2, 30)) / rndLowerTime;

    std::cout << "Buffer size: 2^" << p << " (" << bytes << "b): sequential "
        << seqBandwidth << " GB/s (" << seqLoBandwidth << "-" << seqHiBandwidth
        << "GB/s); random " << rndBandwidth << " GB/s (" << rndLoBandwidth
        << "-" << rndHiBandwidth << " GB/s)." << std::endl;
    of << p << "," << bytes << "," << seqBandwidth << "," << seqLoBandwidth
        << "," << seqHiBandwidth << "," << rndBandwidth << "," << rndLoBandwidth
        << "," << rndHiBandwidth << std::endl;
  }
}
