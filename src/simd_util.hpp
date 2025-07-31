#ifndef PNGRAM_SIMD_UTIL_HPP
#define PNGRAM_SIMD_UTIL_HPP

#include <sstream>
#include <iostream>
#include <iomanip>

typedef uint64_t u512 __attribute__((vector_size(64)));
typedef  int64_t i512 __attribute__((vector_size(64)));
typedef uint32_t u32_512 __attribute__((vector_size(64)));
typedef  int32_t i32_512 __attribute__((vector_size(64)));

#define PRINT_U512(x, name) \
    { \
      std::ostringstream oss; \
      oss << name << ": { " << x[0] << ", " << x[1] \
                    << ", " << x[2] << ", " << x[3] \
                    << ", " << x[4] << ", " << x[5] \
                    << ", " << x[6] << ", " << x[7] << " }\n"; \
      std::cout << oss.str(); \
    }

#define PRINT_U512_HEX(x, name) \
    { \
      std::ostringstream oss; \
      oss << name << std::hex << std::setfill('0') << ": " \
                    << "{ 0x" << std::setw(16) << x[0] << ", 0x" << std::setw(16) << x[1] \
                    << ", 0x" << std::setw(16) << x[2] << ", 0x" << std::setw(16) << x[3] \
                    << ", 0x" << std::setw(16) << x[4] << ", 0x" << std::setw(16) << x[5] \
                    << ", 0x" << std::setw(16) << x[6] << ", 0x" << std::setw(16) << x[7] << " }\n"; \
      std::cout << oss.str(); \
    }

#define PRINT_U32_512_HEX(x, name) \
    { \
      std::ostringstream oss; \
      oss << name << std::hex << std::setfill('0') << ": " \
                    << "{ 0x" << std::setw(8) << x[0] << ", 0x" << std::setw(8) << x[1] \
                    << ", 0x" << std::setw(8) << x[2] << ", 0x" << std::setw(8) << x[3] \
                    << ", 0x" << std::setw(8) << x[4] << ", 0x" << std::setw(8) << x[5] \
                    << ", 0x" << std::setw(8) << x[6] << ", 0x" << std::setw(8) << x[7] \
                    << ", 0x" << std::setw(8) << x[8] << ", 0x" << std::setw(8) << x[9] \
                    << ", 0x" << std::setw(8) << x[10] << ", 0x" << std::setw(8) << x[11] \
                    << ", 0x" << std::setw(8) << x[12] << ", 0x" << std::setw(8) << x[13] \
                    << ", 0x" << std::setw(8) << x[14] << ", 0x" << std::setw(8) << x[15] << " }\n"; \
      std::cout << oss.str(); \
    }

#endif
