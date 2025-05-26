#pragma once

#include <optional>

#include "vellum/vellum_object.h"
#include "vellum/vellum_value.h"

namespace vellum {
class Resolver {
 public:
  Resolver(VellumObject object) : object(std::move(object)) {}

  void addFunction(VellumFunction function) {
    object.addFunction(std::move(function));
  }

  void addProperty(VellumProperty property) {
    object.addProperty(std::move(property));
  }

  void addVariable(VellumVariable variable) {
    object.addVariable(std::move(variable));
  }

  void importObject(VellumObject importedObject) {
    importedObjects.push_back(std::move(importedObject));
  }

  std::optional<VellumValue> resolveIdentifier(
      VellumIdentifier identifier) const;

  std::optional<VellumValue> resolveProperty(VellumIdentifier object,
                                             VellumIdentifier member) const;

  std::optional<VellumFunction> resolveFunction(
      VellumIdentifier object, VellumIdentifier function) const;

 private:
  VellumObject object;

  std::vector<VellumObject> importedObjects;
};
}  // namespace vellum