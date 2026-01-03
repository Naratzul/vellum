#include "vellum_property.h"

namespace vellum {
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