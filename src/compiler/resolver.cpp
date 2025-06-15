#include "resolver.h"

namespace vellum {
void Resolver::startFunction(const VellumFunction& func) {
  currentFunction = func;
  pushScope();
  for (const auto& param : func.getParameters()) {
    pushLocalVar(param);
  }
}

void Resolver::endFunction() {
  assert(currentFunction.has_value());
  popScope();
  currentFunction = std::nullopt;
}

void Resolver::pushScope() { scopes.emplace_back(); }
void Resolver::popScope() { scopes.pop_back(); }

void Resolver::pushLocalVar(const VellumVariable& var) {
  auto& scope = scopes.back();
  if (scope.contains(var.getName())) {
    errorHandler->errorAt(
        Token(),
        std::format("Variable with name '{}' aleady defined in this scope.",
                    var.getName().toString()));
    return;
  }
  scopes.back().pushVar(var);
}

void Resolver::popLocalVar() { scopes.back().popVar(); }
void Resolver::popLocalVar(int count) { scopes.back().popVar(count); }

std::optional<VellumValue> Resolver::resolveIdentifier(
    VellumIdentifier identifier) const {
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    if (auto var = it->getVariable(identifier)) {
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
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    if (auto var = it->getVariable(name)) {
      return var;
    }
  }
  return object.findVariable(name);
}

}  // namespace vellum