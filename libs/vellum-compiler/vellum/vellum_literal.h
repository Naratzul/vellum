#pragma once

#include <optional>
#include <ostream>
#include <string_view>
#include <variant>

#include "common/types.h"
#include "pex/pex_value.h"

namespace vellum {

using common::Opt;
using common::Vec;

namespace pex {
class PexFile;
}

enum class VellumLiteralType { None, Int, Float, Bool, String };

class VellumLiteral {
 public:
  VellumLiteral() = default;
  VellumLiteral(int32_t value) : value(value), type(VellumLiteralType::Int) {};
  VellumLiteral(float value) : value(value), type(VellumLiteralType::Float) {};
  VellumLiteral(bool value) : value(value), type(VellumLiteralType::Bool) {};
  VellumLiteral(std::string_view value)
      : value(value), type(VellumLiteralType::String) {};

  VellumLiteralType getType() const { return type; }

  int32_t asInt() const { return std::get<int32_t>(value); }
  float asFloat() const { return std::get<float>(value); }
  bool asBool() const { return std::get<bool>(value); }
  std::string_view asString() const {
    return std::get<std::string_view>(value);
  }

 private:
  std::variant<int32_t, float, bool, std::string_view> value;
  VellumLiteralType type = VellumLiteralType::None;
};

VellumLiteral makeDefaultLiteral(VellumLiteralType type);
pex::PexValue makePexValue(VellumLiteral value, pex::PexFile& file);

std::string_view literalTypeToString(VellumLiteralType type);
Opt<VellumLiteralType> literalTypeFromString(std::string_view name);

bool operator==(const VellumLiteral& lhs, const VellumLiteral& rhs);
bool operator!=(const VellumLiteral& lhs, const VellumLiteral& rhs);
std::ostream& operator<<(std::ostream& os, const VellumLiteral& value);
}  // namespace vellum