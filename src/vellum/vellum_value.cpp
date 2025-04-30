#include "vellum_value.h"

#include "pex/pex_file.h"

namespace vellum {
VellumValueType typeFromString(std::string_view name) {
  if (name == "String") {
    return VellumValueType::String;
  } else if (name == "Int") {
    return VellumValueType::Int;
  } else if (name == "Float") {
    return VellumValueType::Float;
  } else if (name == "Bool") {
    return VellumValueType::Bool;
  }
  return VellumValueType::None;
}

VellumValue makeDefaultValue(VellumValueType type) {
  switch (type) {
    case VellumValueType::String:
      return VellumValue(std::string_view(""));
    case VellumValueType::Int:
      return VellumValue(int32_t(0));
    case VellumValueType::Float:
      return VellumValue(float(0.0f));
    case VellumValueType::Bool:
      return VellumValue(bool(false));
    case VellumValueType::Identifier:
    case VellumValueType::None:
      return VellumValue();
  }
}

std::optional<pex::PexValue> makePexValue(VellumValue value,
                                          pex::PexFile& file) {
  switch (value.getType()) {
    case VellumValueType::String: {
      const pex::PexString pexValue = file.getString(value.asString());
      return pex::PexValue(pexValue);
    }
    case VellumValueType::Identifier:
      return pex::PexValue(
          pex::PexIdentifier(file.getString(value.asIdentifier().getValue())));
    case VellumValueType::Int:
      return pex::PexValue(value.asInt());
    case VellumValueType::Float:
      return pex::PexValue(value.asFloat());
    case VellumValueType::Bool:
      return pex::PexValue(value.asBool());
    case VellumValueType::None:
      return pex::PexValue();
  }

  return std::nullopt;
}
}  // namespace vellum