#pragma once

#include <bit>
#include <cstdint>
#include <utility>
#include <version>

namespace vellum {
namespace common {

template <typename T>
inline T swapBytes(T value) {
  static_assert(sizeof(T) > 1);

#ifdef __APPLE__
  if constexpr (sizeof(T) == 2) {
    return __builtin_bswap16(value);
  } else if constexpr (sizeof(T) == 4) {
    return __builtin_bswap32(value);
  } else if constexpr (sizeof(T) == 8) {
    return __builtin_bswap64(value);
  } else {
    static_assert(false, "Unsupported type for byte swap.");
  }
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