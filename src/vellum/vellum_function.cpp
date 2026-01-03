#include "vellum_function.h"

namespace vellum {

VellumFunctionCall VellumFunctionCall::methodCall(VellumIdentifier object,
                                                  VellumType objectType,
                                                  VellumIdentifier function) {
  return VellumFunctionCall(objectType, object, function);
}

VellumFunctionCall VellumFunctionCall::staticCall(VellumType objectType,
                                                  VellumIdentifier function) {
  return VellumFunctionCall(objectType, std::nullopt, function);
}

bool operator==(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs) {
  return lhs.isStatic() == rhs.isStatic() &&
         lhs.getObjectType() == rhs.getObjectType() &&
         lhs.getFunction() == rhs.getFunction();
}

bool operator!=(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumFunctionCall& value) {
  os << value.getObjectType() << "." << value.getFunction() << "()";
  return os;
}

bool operator==(const VellumFunction& lhs, const VellumFunction& rhs) {
  return lhs.isStatic() == rhs.isStatic() && lhs.getName() == rhs.getName() &&
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