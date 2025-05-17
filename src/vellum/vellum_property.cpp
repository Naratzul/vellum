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
}  // namespace vellum