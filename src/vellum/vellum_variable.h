#pragma once

#include "common/types.h"
#include "vellum_identifier.h"
#include "vellum_literal.h"
#include "vellum_type.h"

namespace vellum {
using common::Opt;

class VellumVariable {
 public:
  VellumVariable(VellumIdentifier name, VellumType type)
      : name(name), type(type) {}
  VellumVariable(VellumIdentifier name, VellumType type,
                 Opt<VellumLiteral> defaultValue)
      : name(name), type(type), defaultValue(std::move(defaultValue)) {}

  VellumIdentifier getName() const { return name; }
  VellumType getType() const { return type; }
  const Opt<VellumLiteral>& getDefaultValue() const { return defaultValue; }

 private:
  VellumIdentifier name;
  VellumType type;
  Opt<VellumLiteral> defaultValue;
};

bool operator==(const VellumVariable& lhs, const VellumVariable& rhs);
bool operator!=(const VellumVariable& lhs, const VellumVariable& rhs);
std::ostream& operator<<(std::ostream& os, const VellumVariable& value);
}  // namespace vellum