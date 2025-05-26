#pragma once

#include "vellum_identifier.h"
#include "vellum_type.h"

namespace vellum {
class VellumPropertyAccess {
 public:
  VellumPropertyAccess(VellumIdentifier object, VellumIdentifier property)
      : object(object), property(property) {}

  VellumIdentifier getObject() const { return object; }
  VellumIdentifier getProperty() const { return property; }

 private:
  VellumIdentifier object;
  VellumIdentifier property;
};

bool operator==(const VellumPropertyAccess& lhs,
                const VellumPropertyAccess& rhs);
bool operator!=(const VellumPropertyAccess& lhs,
                const VellumPropertyAccess& rhs);
std::ostream& operator<<(std::ostream& os, const VellumPropertyAccess& value);

class VellumProperty {
 public:
  VellumProperty(VellumIdentifier name, VellumType type)
      : name(name), type(type) {}

  VellumIdentifier getName() const { return name; }

  VellumType getType() const { return type; }

 private:
  VellumIdentifier name;
  VellumType type;
};

bool operator==(const VellumProperty& lhs, const VellumProperty& rhs);
bool operator!=(const VellumProperty& lhs, const VellumProperty& rhs);
std::ostream& operator<<(std::ostream& os, const VellumProperty& value);
}  // namespace vellum