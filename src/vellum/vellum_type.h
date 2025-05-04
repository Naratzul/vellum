#pragma once

#include <optional>
#include <string_view>
#include <variant>

namespace vellum {

enum class VellumValueType { None, Int, Float, Bool, String, Identifier };

class VellumIdentifier {
 public:
  explicit VellumIdentifier(std::string_view value) : value(value) {}
  std::string_view getValue() const { return value; }
  std::string_view toString() const { return value; }

 private:
  std::string_view value;
};

bool operator==(const VellumIdentifier& lhs, const VellumIdentifier& rhs);
bool operator!=(const VellumIdentifier& lhs, const VellumIdentifier& rhs);
std::ostream& operator<<(std::ostream& os, const VellumIdentifier& id);

enum class VellumTypeState { Unresolved, Literal, Identifier };

class VellumType {
 public:
  static VellumType literal(VellumValueType type);
  static VellumType identifier(VellumIdentifier identifier);
  static VellumType unresolved(std::string_view type);

  VellumTypeState getState() const { return state; }
  bool isResolved() const { return state != VellumTypeState::Unresolved; }

  std::string_view asRawType() const;
  VellumValueType asLiteralType() const;
  VellumIdentifier asIdentifier() const;

  std::string_view toString() const;

 private:
  VellumTypeState state;
  std::variant<std::string_view, VellumValueType, VellumIdentifier> type;

  explicit VellumType(std::string_view type);
  explicit VellumType(VellumValueType type);
  explicit VellumType(VellumIdentifier type);
};

bool operator==(const VellumType& lhs, const VellumType& rhs);
bool operator!=(const VellumType& lhs, const VellumType& rhs);

std::string_view valueTypeToString(VellumValueType type);
std::optional<VellumValueType> typeFromString(std::string_view name);
}  // namespace vellum