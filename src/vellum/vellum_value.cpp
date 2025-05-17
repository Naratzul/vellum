#include "vellum_value.h"

#include "pex/pex_file.h"

namespace vellum {

pex::PexValue makePexValue(VellumValue value, pex::PexFile& file) {
  switch (value.getType()) {
    case VellumValueType::None:
      return pex::PexValue();
    case VellumValueType::Identifier:
      return pex::PexValue(
          pex::PexIdentifier(file.getString(value.asIdentifier().getValue())));
    case VellumValueType::Literal:
      return makePexValue(value.asLiteral(), file);
  }

  assert(false && "Unknown value type");
  return pex::PexValue();
}

bool operator==(const VellumValue& lhs, const VellumValue& rhs) {
  if (lhs.getType() != rhs.getType()) {
    return false;
  }

  switch (lhs.getType()) {
    case VellumValueType::Function:
      return lhs.asFunction() == rhs.asFunction();
    case VellumValueType::Identifier:
      return lhs.asIdentifier() == rhs.asIdentifier();
    case VellumValueType::Literal:
      return lhs.asLiteral() == rhs.asLiteral();
    case VellumValueType::None:
      return true;
    case VellumValueType::PropertyAccess:
      return lhs.asPropertyAccess() == rhs.asPropertyAccess();
  }

  assert(false && "Unknown value type");
}

bool operator!=(const VellumValue& lhs, const VellumValue& rhs) {
  return !(lhs == rhs);
}

static std::string_view valueTypeToString(VellumValueType type) {
  switch (type) {
    case VellumValueType::Function:
      return "Function";
    case VellumValueType::Identifier:
      return "Identifier";
    case VellumValueType::Literal:
      return "Literal";
    case VellumValueType::None:
      return "None";
    case VellumValueType::PropertyAccess:
      return "PropertyAccess";
  }
}

std::ostream& operator<<(std::ostream& os, const VellumValue& value) {
  os << valueTypeToString(value.getType()) << ": ";
  switch (value.getType()) {
    case VellumValueType::Function:
      os << value.asFunction();
    case VellumValueType::Identifier:
      os << value.asIdentifier();
      break;
    case VellumValueType::Literal:
      os << value.asLiteral();
      break;
    case VellumValueType::None:
      os << "None";
      break;
    case VellumValueType::PropertyAccess:
      os << value.asPropertyAccess();
      break;
  }
  return os;
}
}  // namespace vellum