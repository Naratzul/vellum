#include "vellum_function.h"

namespace vellum {
bool operator==(const VellumFunction& lhs, const VellumFunction& rhs) {
  return lhs.isStatic() == rhs.isStatic() &&
         lhs.getObject() == rhs.getObject() &&
         lhs.getFunction() == rhs.getFunction() &&
         lhs.getReturnType() == rhs.getReturnType();
}

bool operator!=(const VellumFunction& lhs, const VellumFunction& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumFunction& value) {
  os << value.getObject() << "." << value.getFunction() << "()";
  return os;
}
}  // namespace vellum