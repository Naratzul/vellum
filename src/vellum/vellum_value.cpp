#include "vellum_value.h"

#include "pex/pex_file.h"

namespace vellum {
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

  assert(false && "Unknown value type");

  return std::nullopt;
}

bool operator==(const VellumValue& lhs, const VellumValue& rhs) {
  if (lhs.getType() != rhs.getType()) {
    return false;
  }

  switch (lhs.getType()) {
    case VellumValueType::Bool:
      return lhs.asBool() == rhs.asBool();
    case VellumValueType::Float:
      return lhs.asFloat() == rhs.asFloat();
    case VellumValueType::Identifier:
      return lhs.asIdentifier() == rhs.asIdentifier();
    case VellumValueType::Int:
      return lhs.asInt() == rhs.asInt();
    case VellumValueType::String:
      return lhs.asString() == rhs.asString();
    case VellumValueType::None:
      return true;
  }

  assert(false && "Unknown value type");
}

bool operator!=(const VellumValue& lhs, const VellumValue& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumValue& value) {
  os << valueTypeToString(value.getType()) << ": ";
  switch (value.getType()) {
    case VellumValueType::Bool:
      os << (value.asBool() ? "true" : "false");
      break;
    case VellumValueType::Float:
      os << value.asFloat() << "f";
      break;
    case VellumValueType::Identifier:
      os << value.asIdentifier();
      break;
    case VellumValueType::Int:
      os << value.asInt();
      break;
    case VellumValueType::String:
      os << "\"" << value.asString() << "\"";
      break;
    case VellumValueType::None:
      os << "None";
      break;
  }
  return os;
}
}  // namespace vellum