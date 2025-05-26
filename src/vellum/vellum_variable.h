#pragma once

#include "vellum_identifier.h"
#include "vellum_type.h"

namespace vellum {
class VellumVariable {
 public:
  VellumVariable(VellumIdentifier name, VellumType type)
      : name(name), type(type) {}

  VellumIdentifier getName() const { return name; }
  VellumType getType() const { return type; }

 private:
  VellumIdentifier name;
  VellumType type;
};

bool operator==(const VellumVariable& lhs, const VellumVariable& rhs);
bool operator!=(const VellumVariable& lhs, const VellumVariable& rhs);
std::ostream& operator<<(std::ostream& os, const VellumVariable& value);
}  // namespace vellum