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

VellumFunctionCall VellumFunctionCall::parentCall(VellumType parentType,
                                                  VellumIdentifier function) {
  return VellumFunctionCall(parentType, std::nullopt, function, true);
}

bool operator==(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs) {
  return lhs.isStatic() == rhs.isStatic() &&
         lhs.isParentCall() == rhs.isParentCall() &&
         lhs.getObjectType() == rhs.getObjectType() &&
         lhs.getFunction() == rhs.getFunction();
}

bool operator!=(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumFunctionCall& value) {
  if (value.isParentCall()) {
    os << "super." << value.getFunction() << "()";
  } else {
    os << value.getObjectType() << "." << value.getFunction() << "()";
  }
  return os;
}

bool operator==(const VellumFunction& lhs, const VellumFunction& rhs) {
  return lhs.isStatic() == rhs.isStatic() && lhs.getName() == rhs.getName() &&
         lhs.getReturnType() == rhs.getReturnType() &&
         lhs.getParameters() == rhs.getParameters() &&
         lhs.getIntrinsicKind() == rhs.getIntrinsicKind();
}

bool operator!=(const VellumFunction& lhs, const VellumFunction& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const VellumFunction& value) {
  os << "fun " << value.getName() << "()"
     << " -> " << value.getReturnType();
  return os;
}

bool VellumFunction::matchSignature(const VellumFunction& other) const {
  if (staticFunc != other.staticFunc) {
    return false;
  }

  if (returnType != other.returnType) {
    return false;
  }

  if (parameters.size() != other.parameters.size()) {
    return false;
  }

  for (size_t i = 0; i < parameters.size(); ++i) {
    if (parameters[i].getType() != other.parameters[i].getType()) {
      return false;
    }
  }

  return true;
}
}  // namespace vellum