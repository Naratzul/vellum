#pragma once

#include <cassert>
#include <optional>
#include <ostream>
#include <string_view>
#include <variant>

#include "pex/pex_value.h"
#include "vellum_function.h"
#include "vellum_identifier.h"
#include "vellum_literal.h"
#include "vellum_property.h"

namespace vellum {

namespace pex {
class PexFile;
}

enum class VellumValueType {
  None,
  Literal,
  Identifier,
  PropertyAccess,
  Function
};

class VellumValue {
 public:
  VellumValue() = default;
  VellumValue(VellumLiteral value)
      : value(value), type(VellumValueType::Literal) {};
  VellumValue(VellumIdentifier value)
      : value(value), type(VellumValueType::Identifier) {};
  VellumValue(VellumPropertyAccess value)
      : value(value), type(VellumValueType::PropertyAccess) {};
  VellumValue(VellumFunction value)
      : value(value), type(VellumValueType::Function) {};

  VellumValueType getType() const { return type; }

  VellumLiteral asLiteral() const {
    assert(type == VellumValueType::Literal);
    return std::get<VellumLiteral>(value);
  }

  VellumIdentifier asIdentifier() const {
    assert(type == VellumValueType::Identifier);
    return std::get<VellumIdentifier>(value);
  }

  VellumPropertyAccess asPropertyAccess() const {
    assert(type == VellumValueType::PropertyAccess);
    return std::get<VellumPropertyAccess>(value);
  }

  VellumFunction asFunction() const {
    assert(type == VellumValueType::Function);
    return std::get<VellumFunction>(value);
  }

 private:
  std::variant<std::monostate, VellumLiteral, VellumIdentifier,
               VellumPropertyAccess, VellumFunction>
      value;
  VellumValueType type = VellumValueType::None;
};

pex::PexValue makePexValue(VellumValue value, pex::PexFile& file);

bool operator==(const VellumValue& lhs, const VellumValue& rhs);
bool operator!=(const VellumValue& lhs, const VellumValue& rhs);
std::ostream& operator<<(std::ostream& os, const VellumValue& value);
}  // namespace vellum