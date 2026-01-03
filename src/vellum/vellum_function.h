#pragma once

#include <cassert>
#include <optional>
#include <vector>

#include "common/types.h"
#include "vellum_identifier.h"
#include "vellum_type.h"
#include "vellum_variable.h"

namespace vellum {
using common::Opt;
using common::Vec;
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
  VellumFunctionCall(VellumType objectType, Opt<VellumIdentifier> object,
                     VellumIdentifier function)
      : objectType(objectType), function(function), object(object) {}

  VellumType objectType;
  VellumIdentifier function;
  Opt<VellumIdentifier> object;
};

bool operator==(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs);
bool operator!=(const VellumFunctionCall& lhs, const VellumFunctionCall& rhs);
std::ostream& operator<<(std::ostream& os, const VellumFunctionCall& value);

class VellumFunction {
 public:
  VellumFunction(VellumIdentifier name, VellumType returnType,
                 Vec<VellumVariable> parameters, bool staticFunc)
      : name(name),
        returnType(returnType),
        parameters(parameters),
        staticFunc(staticFunc) {}

  VellumIdentifier getName() const { return name; }
  VellumType getReturnType() const { return returnType; }
  const Vec<VellumVariable>& getParameters() const { return parameters; }

  int getArity() const { return parameters.size(); }
  bool isStatic() const { return staticFunc; }

 private:
  VellumIdentifier name;
  VellumType returnType;
  Vec<VellumVariable> parameters;
  bool staticFunc;
};

bool operator==(const VellumFunction& lhs, const VellumFunction& rhs);
bool operator!=(const VellumFunction& lhs, const VellumFunction& rhs);
std::ostream& operator<<(std::ostream& os, const VellumFunction& value);
}  // namespace vellum