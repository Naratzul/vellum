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
    case VellumValueType::FunctionCall:
      return lhs.asFunctionCall() == rhs.asFunctionCall();
    case VellumValueType::Identifier:
      return lhs.asIdentifier() == rhs.asIdentifier();
    case VellumValueType::Literal:
      return lhs.asLiteral() == rhs.asLiteral();
    case VellumValueType::None:
      return true;
    case VellumValueType::Property:
      return lhs.asProperty() == rhs.asProperty();
    case VellumValueType::PropertyAccess:
      return lhs.asPropertyAccess() == rhs.asPropertyAccess();
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
    case VellumValueType::FunctionCall:
      return "FunctionCall";
    case VellumValueType::Identifier:
      return "Identifier";
    case VellumValueType::Literal:
      return "Literal";
    case VellumValueType::None:
      return "None";
    case VellumValueType::Property:
      return "Property";
    case VellumValueType::PropertyAccess:
      return "PropertyAccess";
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
    case VellumValueType::FunctionCall:
      os << value.asFunctionCall();
      break;
    case VellumValueType::Identifier:
      os << value.asIdentifier();
      break;
    case VellumValueType::Literal:
      os << value.asLiteral();
      break;
    case VellumValueType::None:
      os << "None";
      break;
    case VellumValueType::Property:
      os << value.asProperty();
      break;
    case VellumValueType::PropertyAccess:
      os << value.asPropertyAccess();
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