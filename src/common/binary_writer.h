#pragma once

#include <cassert>
#include <limits>
#include <ostream>

#include "byte_swap.h"

namespace vellum {
namespace common {

enum class Endianness { Little, Big };

class BinaryWriter {
 public:
  explicit BinaryWriter(std::ostream& stream, Endianness endiannes)
      : stream(stream), endiannes(endiannes) {}

  template <typename T>
  BinaryWriter& operator<<(T value);

 protected:
  std::ostream& stream;
  Endianness endiannes;

  template <typename T>
  T swapBytes(T value);
};

template <typename T>
inline T BinaryWriter::swapBytes(T value) {
  return endiannes == Endianness::Little ? value : common::swapBytes(value);
}

template <typename T>
inline BinaryWriter& BinaryWriter::operator<<(T value) {
  static_assert(std::is_same_v<T, void>,
                "Unsupported type passed to BinaryWriter.");
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(int8_t value) {
  stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(uint8_t value) {
  stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(int16_t value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(uint16_t value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(int32_t value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(uint32_t value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(int64_t value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(uint64_t value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(float value) {
  const auto swapped = swapBytes(value);
  stream.write(reinterpret_cast<const char*>(&swapped), sizeof(value));
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(bool value) {
  *this << (uint8_t)(value ? 0x01 : 0x00);
  return *this;
}

template <>
inline BinaryWriter& BinaryWriter::operator<<(std::string_view value) {
  assert(value.length() <= std::numeric_limits<uint16_t>::max());
  *this << (uint16_t)value.length();
  if (value.length() > 0) {
    stream.write(value.data(), value.length());
  }
  return *this;
}
}  // namespace common
}  // namespace vellum