#pragma once

#include <optional>
#include <ostream>
#include <string_view>
#include <variant>

#include "pex/pex_value.h"
#include "vellum_type.h"

namespace vellum {

namespace pex {
class PexFile;
}

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

VellumValue makeDefaultValue(VellumValueType type);

std::optional<pex::PexValue> makePexValue(VellumValue value,
                                          pex::PexFile& file);

bool operator==(const VellumValue& lhs, const VellumValue& rhs);
bool operator!=(const VellumValue& lhs, const VellumValue& rhs);
std::ostream& operator<<(std::ostream& os, const VellumValue& value);
}  // namespace vellum