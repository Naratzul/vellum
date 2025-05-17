#pragma once

#include "vellum_identifier.h"

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
}  // namespace vellum