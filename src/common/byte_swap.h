#pragma once

#include <bit>
#include <cstdint>
#include <utility>
#include <version>

namespace vellum {
namespace common {

template <typename T>
inline T swapBytes(T value) {
#ifdef __APPLE__
  return __builtin_bswap64(value);
#else
  return std::byteswap(value);
#endif
}

template <>
inline float swapBytes(float value) {
  const auto intRepr = std::bit_cast<uint32_t>(value);
  const auto swappedInt = swapBytes(intRepr);
  return std::bit_cast<float>(swappedInt);
}

}  // namespace common
}  // namespace vellum