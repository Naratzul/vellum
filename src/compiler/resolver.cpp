#include "resolver.h"

namespace vellum {
void Resolver::pushFunction(const VellumFunction& func) {
  functions.push_back(func);

  for (const auto& var : func.getParameters()) {
    pushLocalVar(var);
  }
}

void Resolver::popFunction() {
  for (int i = 0; i < functions.back().getParameters().size(); ++i) {
    popLocalVar();
  }
  functions.pop_back();
}

void Resolver::pushLocalVar(const VellumVariable& var) {
  localVars.push_back(var);
}

void Resolver::popLocalVar() { localVars.pop_back(); }

std::optional<VellumValue> Resolver::resolveIdentifier(
    VellumIdentifier identifier) const {
  for (const auto& var : localVars) {
    if (var.getName() == identifier) {
      return var;
    }
  }

  return object.findIdentifier(identifier);
}

std::optional<VellumValue> Resolver::resolveProperty(
    VellumIdentifier object, VellumIdentifier member) const {
  if (object == VellumIdentifier("self")) {
    return this->object.findProperty(member);
  }

  for (const auto& importedObject : importedObjects) {
    if (importedObject.getName() == object) {
      return importedObject.findProperty(member);
    }
  }

  return resolveFunction(object, member);
}

std::optional<VellumFunction> Resolver::resolveFunction(
    VellumIdentifier object, VellumIdentifier function) const {
  if (object == VellumIdentifier("self")) {
    return this->object.findFunction(function);
  }

  for (const auto& importedObject : importedObjects) {
    if (importedObject.getName() == object) {
      return importedObject.findFunction(function);
    }
  }

  return std::nullopt;
}

std::optional<VellumVariable> Resolver::resolveVariable(
    VellumIdentifier name) const {
  return object.findVariable(name);
}

}  // namespace vellum