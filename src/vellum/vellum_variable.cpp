#include "vellum_variable.h"

namespace vellum {
bool operator==(const VellumVariable& lhs, const VellumVariable& rhs) {
  return lhs.getName() == rhs.getName() && lhs.getType() == rhs.getType();
}

bool operator!=(const VellumVariable& lhs, const VellumVariable& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumVariable& value) {
  os << "var " << value.getName() << ": " << value.getType();
  return os;
}
}  // namespace vellum