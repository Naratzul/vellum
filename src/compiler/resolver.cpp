#include "resolver.h"

namespace vellum {

std::vector<VellumObject> Resolver::builtinObjects;

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

  if (auto id = object.findIdentifier(identifier)) {
    return id;
  }

  return resolveScriptType(identifier);
}

std::optional<VellumValue> Resolver::resolveProperty(
    VellumType type, VellumIdentifier member) const {
  if (type == object.getType()) {
    return this->object.findProperty(member);
  }

  for (const auto& object : builtinObjects) {
    if (object.getType() == type) {
      return object.findProperty(member);
    }
  }

  for (const auto& importedObject : importedObjects) {
    if (importedObject.getType() == type) {
      return importedObject.findProperty(member);
    }
  }

  return std::nullopt;
}

std::optional<VellumFunction> Resolver::resolveFunction(
    VellumType type, VellumIdentifier function) const {
  if (type == object.getType()) {
    return this->object.findFunction(function);
  }

  for (const auto& importedObject : importedObjects) {
    if (importedObject.getType() == type) {
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

std::optional<VellumType> Resolver::resolveScriptType(
    VellumIdentifier identifier) const {
  auto type = VellumType::identifier(identifier);

  if (object.getType() == type) {
    return object.getType();
  }

  for (const auto& object : importedObjects) {
    if (object.getType() == type) {
      return object.getType();
    }
  }

  return std::nullopt;
}
}  // namespace vellum