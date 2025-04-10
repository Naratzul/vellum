#pragma once

#include <bit>
#include <cstdint>

namespace vellum {
namespace common {

template <typename T>
inline T swapBytes(T value) {
  return std::byteswap(value);
}

template <>
inline float swapBytes(float value) {
  const auto intRepr = std::bit_cast<uint32_t>(value);
  const auto swappedInt = std::byteswap(intRepr);
  return std::bit_cast<float>(swappedInt);
}

}  // namespace common
}  // namespace vellum