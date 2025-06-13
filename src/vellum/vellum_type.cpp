#include "vellum_type.h"

#include <cassert>

namespace vellum {

bool operator==(const VellumType& lhs, const VellumType& rhs) {
  if (lhs.getState() != rhs.getState()) {
    return false;
  }

  switch (lhs.getState()) {
    case VellumTypeState::Unresolved:
      return lhs.asRawType() == rhs.asRawType();
    case VellumTypeState::Identifier:
      return lhs.asIdentifier() == rhs.asIdentifier();
    case VellumTypeState::Literal:
      return lhs.asLiteralType() == rhs.asLiteralType();
    case VellumTypeState::None:
      return true;
    default:
      assert(false && "Unknown vellum type state.");
  }
}

bool operator!=(const VellumType& lhs, const VellumType& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumType& value) {
  os << value.toString();
  return os;
}

VellumType VellumType::literal(VellumLiteralType type) {
  return VellumType(type);
}

VellumType VellumType::none() { return VellumType(); }

VellumType VellumType::identifier(VellumIdentifier identifier) {
  return VellumType(identifier);
}

VellumType VellumType::unresolved(std::string_view type) {
  return VellumType(type);
}

std::string_view VellumType::asRawType() const {
  assert(state == VellumTypeState::Unresolved);
  return std::get<std::string_view>(type);
}

VellumLiteralType VellumType::asLiteralType() const {
  assert(state == VellumTypeState::Literal);
  return std::get<VellumLiteralType>(type);
}

VellumIdentifier VellumType::asIdentifier() const {
  assert(state == VellumTypeState::Identifier);
  return std::get<VellumIdentifier>(type);
}

std::string_view VellumType::toString() const {
  switch (state) {
    case VellumTypeState::Identifier:
      return asIdentifier().toString();
    case VellumTypeState::Literal:
      return literalTypeToString(asLiteralType());
    case VellumTypeState::Unresolved:
      return asRawType();
    case VellumTypeState::None:
      return "None";
  }
}

bool VellumType::isInt() const {
  return getState() == VellumTypeState::Literal &&
         asLiteralType() == VellumLiteralType::Int;
}

bool VellumType::isFloat() const {
  return getState() == VellumTypeState::Literal &&
         asLiteralType() == VellumLiteralType::Float;
}

bool VellumType::isString() const {
  return getState() == VellumTypeState::Literal &&
         asLiteralType() == VellumLiteralType::String;
}

bool VellumType::isBool() const {
  return getState() == VellumTypeState::Literal &&
         asLiteralType() == VellumLiteralType::Bool;
  ;
}

VellumType::VellumType(std::string_view type)
    : state(VellumTypeState::Unresolved), type(type) {}

VellumType::VellumType(VellumLiteralType type)
    : state(VellumTypeState::Literal), type(type) {}

VellumType::VellumType(VellumIdentifier type)
    : state(VellumTypeState::Identifier), type(type) {}
}  // namespace vellum