#include "vellum_value.h"

#include "common/string_utils.h"
#include "pex/pex_file.h"

namespace vellum {

pex::PexValue makePexValue(VellumValue value, pex::PexFile& file) {
  switch (value.getType()) {
    case VellumValueType::Identifier:
      return pex::PexValue(pex::PexIdentifier(
          file.getString(common::normalizeToLower(value.asIdentifier().getValue()))));
    case VellumValueType::Literal:
      return makePexValue(value.asLiteral(), file);
    default:
      assert(false && "Unknown value type");
  }

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
    case VellumValueType::Property:
      return lhs.asProperty() == rhs.asProperty();
    case VellumValueType::ScriptType:
      return lhs.asScriptType() == rhs.asScriptType();
    case VellumValueType::Variable:
      return lhs.asVariable() == rhs.asVariable();
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
    case VellumValueType::Property:
      return "Property";
    case VellumValueType::ScriptType:
      return "ScriptType";
    case VellumValueType::Variable:
      return "Variable";
  }
}

std::ostream& operator<<(std::ostream& os, const VellumValue& value) {
  os << valueTypeToString(value.getType()) << ": ";
  switch (value.getType()) {
    case VellumValueType::Function:
      os << value.asFunction();
      break;
    case VellumValueType::Identifier:
      os << value.asIdentifier();
      break;
    case VellumValueType::Literal:
      os << value.asLiteral();
      break;
    case VellumValueType::Property:
      os << value.asProperty();
      break;
    case VellumValueType::ScriptType:
      os << value.asScriptType();
      break;
    case VellumValueType::Variable:
      os << value.asVariable();
      break;
  }
  return os;
}
}  // namespace vellum