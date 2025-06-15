#pragma once

#include <vector>

#include "vellum_identifier.h"
#include "vellum_type.h"
#include "vellum_variable.h"

namespace vellum {
class VellumFunctionCall {
 public:
  VellumFunctionCall(VellumIdentifier object, VellumIdentifier function,
                     bool isStatic)
      : object(object),
        function(function),
        returnType(VellumType::unresolved()),
        isStatic_(isStatic) {}

  VellumIdentifier getObject() const { return object; }
  VellumIdentifier getFunction() const { return function; }
  bool isStatic() const { return isStatic_; }

 private:
  VellumIdentifier object;
  VellumIdentifier function;
  VellumType returnType;
  bool isStatic_;
};

bool operator==(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs);
bool operator!=(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs);
std::ostream& operator<<(std::ostream& os, const VellumFunctionCall& value);

class VellumFunction {
 public:
  VellumFunction(VellumIdentifier name, VellumType returnType,
                 std::vector<VellumVariable> parameters)
      : name(name), returnType(returnType), parameters(parameters) {}

  VellumIdentifier getName() const { return name; }
  VellumType getReturnType() const { return returnType; }
  const std::vector<VellumVariable>& getParameters() const {
    return parameters;
  }

  int getArity() const { return parameters.size(); }

 private:
  VellumIdentifier name;
  VellumType returnType;
  std::vector<VellumVariable> parameters;
};

bool operator==(const VellumFunction& lhs, const VellumFunction& rhs);
bool operator!=(const VellumFunction& lhs, const VellumFunction& rhs);
std::ostream& operator<<(std::ostream& os, const VellumFunction& value);
}  // namespace vellum