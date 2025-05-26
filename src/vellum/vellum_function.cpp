#include "vellum_function.h"

namespace vellum {
bool operator==(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs) {
  return lhs.isStatic() == rhs.isStatic() &&
         lhs.getObject() == rhs.getObject() &&
         lhs.getFunction() == rhs.getFunction();
}

bool operator!=(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumFunctionCall& value) {
  os << value.getObject() << "." << value.getFunction() << "()";
  return os;
}

bool operator==(const VellumFunction& lhs, const VellumFunction& rhs) {
  return lhs.getName() == rhs.getName() &&
         lhs.getReturnType() == rhs.getReturnType() &&
         lhs.getParameters() == rhs.getParameters();
}

bool operator!=(const VellumFunction& lhs, const VellumFunction& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumFunction& value) {
  os << "fun " << value.getName() << "()"
     << " -> " << value.getReturnType();
  return os;
}
}  // namespace vellum