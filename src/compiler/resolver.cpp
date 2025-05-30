#include "resolver.h"

namespace vellum {

std::optional<VellumValue> Resolver::resolveIdentifier(
    VellumIdentifier identifier) const {
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