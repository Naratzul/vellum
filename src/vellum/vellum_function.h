#pragma once

#include <cassert>
#include <optional>
#include <vector>

#include "vellum_identifier.h"
#include "vellum_type.h"
#include "vellum_variable.h"

namespace vellum {
class VellumFunctionCall {
 public:
  VellumFunctionCall() = delete;

  static VellumFunctionCall methodCall(VellumIdentifier object,
                                       VellumType objectType,
                                       VellumIdentifier function);

  static VellumFunctionCall staticCall(VellumType objectType,
                                       VellumIdentifier function);

  VellumType getObjectType() const { return objectType; }

  VellumIdentifier getObject() const {
    assert(!isStatic());
    return object.value();
  }

  VellumIdentifier getFunction() const { return function; }

  bool isStatic() const { return object == std::nullopt; }

 private:
  VellumFunctionCall(VellumType objectType,
                     std::optional<VellumIdentifier> object,
                     VellumIdentifier function)
      : object(object), function(function), objectType(objectType) {}

  VellumType objectType;
  VellumIdentifier function;
  std::optional<VellumIdentifier> object;
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