#include "vellum_literal.h"

#include "pex/pex_file.h"

namespace vellum {
VellumLiteral makeDefaultLiteral(VellumLiteralType type) {
  switch (type) {
    case VellumLiteralType::None:
      return VellumLiteral();
    case VellumLiteralType::String:
      return VellumLiteral(std::string_view(""));
    case VellumLiteralType::Int:
      return VellumLiteral(int32_t(0));
    case VellumLiteralType::Float:
      return VellumLiteral(float(0.0f));
    case VellumLiteralType::Bool:
      return VellumLiteral(bool(false));
  }
}

std::string_view literalTypeToString(VellumLiteralType type) {
  switch (type) {
    case VellumLiteralType::None:
      return "None";
    case VellumLiteralType::Int:
      return "Int";
    case VellumLiteralType::Float:
      return "Float";
    case VellumLiteralType::Bool:
      return "Bool";
    case VellumLiteralType::String:
      return "String";
  }
  assert(false && "Unknown literal type");
}

common::Opt<VellumLiteralType> literalTypeFromString(std::string_view name) {
  if (name == "None") {
    return VellumLiteralType::None;
  } else if (name == "String") {
    return VellumLiteralType::String;
  } else if (name == "Int") {
    return VellumLiteralType::Int;
  } else if (name == "Float") {
    return VellumLiteralType::Float;
  } else if (name == "Bool") {
    return VellumLiteralType::Bool;
  }
  return std::nullopt;
}

pex::PexValue makePexValue(VellumLiteral value, pex::PexFile& file) {
  switch (value.getType()) {
    case VellumLiteralType::None:
      return pex::PexValue();
    case VellumLiteralType::String: {
      const pex::PexString pexValue = file.getString(value.asString());
      return pex::PexValue(pexValue);
    }
    case VellumLiteralType::Int:
      return pex::PexValue(value.asInt());
    case VellumLiteralType::Float:
      return pex::PexValue(value.asFloat());
    case VellumLiteralType::Bool:
      return pex::PexValue(value.asBool());
  }

  assert(false && "Unknown literal type");
}

bool operator==(const VellumLiteral& lhs, const VellumLiteral& rhs) {
  if (lhs.getType() != rhs.getType()) {
    return false;
  }

  switch (lhs.getType()) {
    case VellumLiteralType::None:
      return true;
    case VellumLiteralType::Bool:
      return lhs.asBool() == rhs.asBool();
    case VellumLiteralType::Float:
      return lhs.asFloat() == rhs.asFloat();
    case VellumLiteralType::Int:
      return lhs.asInt() == rhs.asInt();
    case VellumLiteralType::String:
      return lhs.asString() == rhs.asString();
  }

  assert(false && "Unknown literal type");
}

bool operator!=(const VellumLiteral& lhs, const VellumLiteral& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumLiteral& value) {
  if (value.getType() != VellumLiteralType::None) {
    os << literalTypeToString(value.getType()) << ": ";
  }

  switch (value.getType()) {
    case VellumLiteralType::Bool:
      os << (value.asBool() ? "true" : "false");
      break;
    case VellumLiteralType::Float:
      os << value.asFloat() << "f";
      break;
    case VellumLiteralType::Int:
      os << value.asInt();
      break;
    case VellumLiteralType::String:
      os << "\"" << value.asString() << "\"";
      break;
    case vellum::VellumLiteralType::None:
      os << "None";
      break;
  }
  return os;
}
}  // namespace vellum