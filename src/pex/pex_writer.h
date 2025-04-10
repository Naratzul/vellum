#pragma once

#include <cassert>
#include <string_view>
#include <vector>

#include "common/binary_writer.h"
#include "game/game_id.h"
#include "pex_string.h"

namespace vellum {
namespace pex {

class PexWriter : public common::BinaryWriter {
 public:
  explicit PexWriter(std::ostream& stream, common::Endianness endiannes)
      : common::BinaryWriter(stream, endiannes) {}

  template <typename T>
  PexWriter& operator<<(T value);
};

template <typename T>
inline PexWriter& PexWriter::operator<<(T value) {
  common::BinaryWriter::operator<<(std::forward<T>(value));
  return *this;
}

template <>
inline PexWriter& PexWriter::operator<<(game::GameID gameID) {
  *this << (uint16_t)gameID;
  return *this;
}

template <>
inline PexWriter& PexWriter::operator<<(PexString string) {
  assert(string.isValid());
  *this << (uint16_t)string.index();
  return *this;
}
};  // namespace pex
};  // namespace vellum