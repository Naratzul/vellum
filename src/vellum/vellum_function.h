#pragma once

#include "vellum_identifier.h"
#include "vellum_type.h"

namespace vellum {
class VellumFunction {
 public:
  VellumFunction(VellumIdentifier object, VellumIdentifier function,
                 bool isStatic)
      : object(object),
        function(function),
        returnType(VellumType::unresolved()),
        isStatic_(isStatic) {}

  VellumIdentifier getObject() const { return object; }
  VellumIdentifier getFunction() const { return function; }
  bool isStatic() const { return isStatic_; }

  VellumType getReturnType() const { return returnType; }
  void setReturnType(VellumType type) { returnType = type; }

 private:
  VellumIdentifier object;
  VellumIdentifier function;
  VellumType returnType;
  bool isStatic_;
};

bool operator==(const VellumFunction& lhs, const VellumFunction& rhs);
bool operator!=(const VellumFunction& lhs, const VellumFunction& rhs);
std::ostream& operator<<(std::ostream& os, const VellumFunction& value);
}  // namespace vellum