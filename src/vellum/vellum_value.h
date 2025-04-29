#pragma once

#include <optional>
#include <string_view>
#include <variant>

#include "pex/pex_value.h"

namespace vellum {

namespace pex {
class PexFile;
}

enum class VellumValueType { None, Int, Float, Bool, String, Identifier };

inline constexpr std::string_view valueTypeToString(VellumValueType type) {
  switch (type) {
    case VellumValueType::None:
      return "None";
    case VellumValueType::Int:
      return "Int";
    case VellumValueType::Float:
      return "Float";
    case VellumValueType::Bool:
      return "Bool";
    case VellumValueType::String:
      return "String";
    case VellumValueType::Identifier:
      return "Identifier";
  }
  return "Unknown type";
}

class VellumType {
 public:
  static VellumType resolved(VellumValueType type);
  static VellumType unresolved(std::string_view typeName);

 private:
};

class VellumIdentifier {
 public:
  explicit VellumIdentifier(std::string_view value) : value(value) {}
  std::string_view getValue() const { return value; }

 private:
  std::string_view value;
};

class VellumValue {
 public:
  VellumValue() = default;
  VellumValue(int32_t value) : value(value), type(VellumValueType::Int) {};
  VellumValue(float value) : value(value), type(VellumValueType::Float) {};
  VellumValue(bool value) : value(value), type(VellumValueType::Bool) {};
  VellumValue(std::string_view value)
      : value(value), type(VellumValueType::String) {};
  VellumValue(VellumIdentifier value)
      : value(value), type(VellumValueType::Identifier) {}

  VellumValueType getType() const { return type; }

  int32_t asInt() const { return std::get<int32_t>(value); }
  float asFloat() const { return std::get<float>(value); }
  bool asBool() const { return std::get<bool>(value); }
  std::string_view asString() const {
    return std::get<std::string_view>(value);
  }
  VellumIdentifier asIdentifier() const {
    return std::get<VellumIdentifier>(value);
  }

 private:
  std::variant<std::monostate, int32_t, float, bool, std::string_view,
               VellumIdentifier>
      value;
  VellumValueType type = VellumValueType::None;
};

VellumValueType typeFromString(std::string_view name);
VellumValue makeDefaultValue(VellumValueType type);

std::optional<pex::PexValue> makePexValue(VellumValue value,
                                          pex::PexFile& file);
}  // namespace vellum