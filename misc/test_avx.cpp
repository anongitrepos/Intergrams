#include <stdint.h>
#include <iostream>
#include <cstring>

int main()
{
  volatile uint64_t x = 0x012847834aa;

  typedef uint64_t u512 __attribute__((vector_size(64)));
  typedef uint8_t uu512 __attribute__((vector_size(64)));
  uu512 results[64];
  memset(results, 0, 256);

  u512 y = { x, x, x, x, x, x, x, x };
  u512 masks = {
       0x03FFF80000000000,
       0x0003FFF800000000,
       0x000003FFF8000000,
       0x00000003FFF80000,
       0x0000000003FFF800,
       0x000000000003FFF8,
       0x00000000000003FF,
       0x0000000000000003 };

  y &= masks;
  u512 rightShifts = { 43, 35, 27, 19, 11, 3, 0, 0 };
  y >>= rightShifts;
  u512 leftShifts = { 0, 0, 0, 0, 0, 0, 5, 13 };
  y <<= leftShifts;
  // now have to add the last bits...
  y = (1 << y);

  for (size_t i = 0; i < 8; ++i)
    y[i] = (y[i] > 0) ? 1 : 0;

  uint64_t sum;
  for (size_t i = 0; i < 8; ++i)
    sum += y[i];

  uint64_t z = y[6];

  std::cout << z << "\n" << sum << "\n";
}
