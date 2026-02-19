#pragma once

#include <iostream>
#include <optional>
#include <string_view>
#include <variant>

#include "common/types.h"
#include "vellum_identifier.h"
#include "vellum_literal.h"

namespace vellum {
using common::Shared;

enum class VellumTypeState { Unresolved, Literal, Identifier, Array };

class VellumType {
 public:
  static VellumType none();
  static VellumType literal(VellumLiteralType type);
  static VellumType identifier(VellumIdentifier identifier);
  static VellumType identifier(std::string_view identifier);
  static VellumType unresolved(std::string_view type = "");
  static VellumType array(VellumType subtype);

  VellumTypeState getState() const { return state; }
  bool isResolved() const { return state != VellumTypeState::Unresolved; }

  std::string_view asRawType() const;
  VellumLiteralType asLiteralType() const;
  VellumIdentifier asIdentifier() const;
  const Shared<VellumType>& asArraySubtype() const;

  bool isInt() const;
  bool isFloat() const;
  bool isString() const;
  bool isBool() const;
  bool isArray() const;
  bool isNone() const;

  std::string_view toString() const;

 private:
  VellumTypeState state = VellumTypeState::Unresolved;
  std::variant<std::string_view, VellumLiteralType, VellumIdentifier,
               Shared<VellumType>>
      type;

  VellumType() = default;
  explicit VellumType(std::string_view type);
  explicit VellumType(VellumLiteralType type);
  explicit VellumType(VellumIdentifier type);
  explicit VellumType(Shared<VellumType> subtype);
};

bool operator==(const VellumType& lhs, const VellumType& rhs);
bool operator!=(const VellumType& lhs, const VellumType& rhs);
std::ostream& operator<<(std::ostream& os, const VellumType& value);
}  // namespace vellum