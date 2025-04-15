#pragma once

#include <string_view>
#include <variant>

namespace vellum {

enum class VellumValueType { Nil, Int, Float, Bool, String };

inline constexpr std::string_view valueTypeToString(VellumValueType type) {
  switch (type) {
    case VellumValueType::Nil:
      return "Nil";
    case VellumValueType::Int:
      return "Int";
    case VellumValueType::Float:
      return "Float";
    case VellumValueType::Bool:
      return "Bool";
    case VellumValueType::String:
      return "String";
  }
  return "Unknown type";
}

class VellumType {
  public:
    static VellumType resolved(VellumValueType type);
    static VellumType unresolved(std::string_view typeName);
  private:
};

class VellumValue {
 public:
  VellumValue() = default;
  VellumValue(int32_t value) : value(value), type(VellumValueType::Int){};
  VellumValue(float value) : value(value), type(VellumValueType::Float){};
  VellumValue(bool value) : value(value), type(VellumValueType::Bool){};
  VellumValue(std::string_view value)
      : value(value), type(VellumValueType::String){};

  VellumValueType getType() const { return type; }

  int32_t asInt() const { return std::get<int32_t>(value); }
  float asFloat() const { return std::get<float>(value); }
  bool asBool() const { return std::get<bool>(value); }
  std::string_view asString() const {
    return std::get<std::string_view>(value);
  }

 private:
  std::variant<std::monostate, int32_t, float, bool, std::string_view> value;
  VellumValueType type = VellumValueType::Nil;
};
}  // namespace vellum