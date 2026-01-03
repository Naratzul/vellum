#pragma once

#include "vellum_identifier.h"
#include "vellum_type.h"

namespace vellum {
class VellumProperty {
 public:
  VellumProperty(VellumIdentifier name, VellumType type, bool readonly)
      : name(name), type(type), readonly(readonly) {}

  VellumIdentifier getName() const { return name; }

  VellumType getType() const { return type; }

  bool isReadonly() const { return readonly; }

 private:
  VellumIdentifier name;
  VellumType type;
  bool readonly;
};

bool operator==(const VellumProperty& lhs, const VellumProperty& rhs);
bool operator!=(const VellumProperty& lhs, const VellumProperty& rhs);
std::ostream& operator<<(std::ostream& os, const VellumProperty& value);
}  // namespace vellum