#include "vellum_property.h"

namespace vellum {
bool operator==(const VellumPropertyAccess& lhs,
                const VellumPropertyAccess& rhs) {
  return lhs.getObject() == rhs.getObject() &&
         lhs.getProperty() == rhs.getProperty();
}

bool operator!=(const VellumPropertyAccess& lhs,
                const VellumPropertyAccess& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumPropertyAccess& value) {
  os << value.getObject() << "." << value.getProperty();
  return os;
}

bool operator==(const VellumProperty& lhs, const VellumProperty& rhs) {
  return lhs.getName() == rhs.getName() && lhs.getType() == rhs.getType() &&
         lhs.isReadonly() == rhs.isReadonly();
}

bool operator!=(const VellumProperty& lhs, const VellumProperty& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumProperty& value) {
  os << value.getName() << ": " << value.getType();
  return os;
}
}  // namespace vellum