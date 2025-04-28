#pragma once

#include <cassert>
#include <string_view>
#include <vector>

#include "common/binary_writer.h"
#include "game/game_id.h"
#include "pex_string.h"
#include "pex_value.h"

namespace vellum {
namespace pex {

class PexWriter final : public common::BinaryWriter {
 public:
  explicit PexWriter(std::ostream& stream, common::Endianness endiannes)
      : common::BinaryWriter(stream, endiannes) {}

  template <typename T>
  PexWriter& operator<<(T value);

  template <typename T>
  PexWriter& operator<<(const std::vector<T>& value);
};

template <typename T>
inline PexWriter& PexWriter::operator<<(T value) {
  common::BinaryWriter::operator<<(std::forward<T>(value));
  return *this;
}

template <typename T>
inline PexWriter& PexWriter::operator<<(const std::vector<T>& value) {
  *this << (uint16_t)value.size();
  for (const auto& v : value) {
    *this << v;
  }
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

template <>
inline PexWriter& PexWriter::operator<<(PexValue value) {
  *this << (uint8_t)value.getType();

  switch (value.getType()) {
    case PexValueType::None:
      break;
    case PexValueType::Identifier:
      *this << value.asIdentifier().getValue();
      break;
    case PexValueType::String:
      *this << value.asString();
      break;
    case PexValueType::Integer:
      *this << (uint32_t)value.asInt();
      break;
    case PexValueType::Float:
      *this << value.asFloat();
      break;
    case PexValueType::Bool:
      *this << value.asBool();
      break;
    default:
      assert(false && "Unsupported pex value type");
  }

  return *this;
}
};  // namespace pex
};  // namespace vellum