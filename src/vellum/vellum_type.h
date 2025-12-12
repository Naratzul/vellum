#pragma once

#include <iostream>
#include <optional>
#include <string_view>
#include <variant>

#include "vellum_identifier.h"
#include "vellum_literal.h"

namespace vellum {

enum class VellumTypeState { Unresolved, None, Literal, Identifier, Array };

class VellumType {
 public:
  static VellumType none();
  static VellumType literal(VellumLiteralType type);
  static VellumType identifier(VellumIdentifier identifier);
  static VellumType unresolved(std::string_view type = "");
  static VellumType array(VellumType subtype);

  VellumTypeState getState() const { return state; }
  bool isResolved() const { return state != VellumTypeState::Unresolved; }

  std::string_view asRawType() const;
  VellumLiteralType asLiteralType() const;
  VellumIdentifier asIdentifier() const;
  const std::shared_ptr<VellumType>& asArraySubtype() const;

  bool isInt() const;
  bool isFloat() const;
  bool isString() const;
  bool isBool() const;
  bool isArray() const;

  std::string_view toString() const;

 private:
  VellumTypeState state = VellumTypeState::None;
  std::variant<std::monostate, std::string_view, VellumLiteralType,
               VellumIdentifier, std::shared_ptr<VellumType>>
      type;

  VellumType() = default;
  explicit VellumType(std::string_view type);
  explicit VellumType(VellumLiteralType type);
  explicit VellumType(VellumIdentifier type);
  explicit VellumType(std::shared_ptr<VellumType> subtype);
};

bool operator==(const VellumType& lhs, const VellumType& rhs);
bool operator!=(const VellumType& lhs, const VellumType& rhs);
std::ostream& operator<<(std::ostream& os, const VellumType& value);
}  // namespace vellum