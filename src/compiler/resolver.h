#pragma once

#include <optional>

#include "compiler_error_handler.h"
#include "scope.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_value.h"

namespace vellum {

class Resolver {
 public:
  Resolver(VellumObject object,
           std::shared_ptr<CompilerErrorHandler> errorHandler)
      : object(std::move(object)), errorHandler(errorHandler) {}

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

  const std::optional<VellumFunction>& getCurrentFunction() {
    return currentFunction;
  }
  void startFunction(const VellumFunction& func);
  void endFunction();

  void pushScope();
  void popScope();

  void pushLocalVar(const VellumVariable& var);
  void popLocalVar();
  void popLocalVar(int count);

  std::optional<VellumValue> resolveIdentifier(
      VellumIdentifier identifier) const;

  std::optional<VellumValue> resolveProperty(VellumIdentifier object,
                                             VellumIdentifier member) const;

  std::optional<VellumFunction> resolveFunction(
      VellumIdentifier object, VellumIdentifier function) const;

  std::optional<VellumVariable> resolveVariable(VellumIdentifier name) const;

 private:
  VellumObject object;
  std::shared_ptr<CompilerErrorHandler> errorHandler;

  std::vector<VellumObject> importedObjects;
  std::vector<Scope> scopes;
  std::optional<VellumFunction> currentFunction;
};
}  // namespace vellum