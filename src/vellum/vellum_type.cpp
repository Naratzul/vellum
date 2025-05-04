#include "vellum_type.h"

#include <cassert>

namespace vellum {
bool operator==(const VellumIdentifier& lhs, const VellumIdentifier& rhs) {
  return lhs.getValue() == rhs.getValue();
}

bool operator!=(const VellumIdentifier& lhs, const VellumIdentifier& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumIdentifier& id) {
  os << id.getValue();
  return os;
}

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
    default:
      assert(false && "Unknown vellum type state.");
  }
}

bool operator!=(const VellumType& lhs, const VellumType& rhs) {
  return !(lhs == rhs);
}

std::string_view valueTypeToString(VellumValueType type) {
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

std::optional<VellumValueType> typeFromString(std::string_view name) {
  if (name == "String") {
    return VellumValueType::String;
  } else if (name == "Int") {
    return VellumValueType::Int;
  } else if (name == "Float") {
    return VellumValueType::Float;
  } else if (name == "Bool") {
    return VellumValueType::Bool;
  }
  return std::nullopt;
}

VellumType VellumType::literal(VellumValueType type) {
  return VellumType(type);
}

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

VellumValueType VellumType::asLiteralType() const {
  assert(state == VellumTypeState::Literal);
  return std::get<VellumValueType>(type);
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
      return valueTypeToString(asLiteralType());
    case VellumTypeState::Unresolved:
      return asRawType();
  }
}

VellumType::VellumType(std::string_view type)
    : state(VellumTypeState::Unresolved), type(type) {}

VellumType::VellumType(VellumValueType type)
    : state(VellumTypeState::Literal), type(type) {}

VellumType::VellumType(VellumIdentifier type)
    : state(VellumTypeState::Identifier), type(type) {}
}  // namespace vellum