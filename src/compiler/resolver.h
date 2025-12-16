#pragma once

#include <optional>

#include "compiler_error_handler.h"
#include "scope.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_value.h"

namespace vellum {

class Resolver {
 public:
  static void addBultinObject(VellumObject object) {
    builtinObjects.push_back(std::move(object));
  }

  Resolver(VellumObject object,
           std::shared_ptr<CompilerErrorHandler> errorHandler)
      : object(std::move(object)), errorHandler(errorHandler) {}

  const VellumObject& getObject() const { return object; }

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

  std::optional<VellumValue> resolveProperty(VellumType type,
                                             VellumIdentifier member) const;

  std::optional<VellumFunction> resolveFunction(
      VellumType type, VellumIdentifier function) const;

  std::optional<VellumVariable> resolveVariable(VellumIdentifier name) const;

  std::optional<VellumType> resolveScriptType(VellumIdentifier identifier) const;

 private:
  VellumObject object;
  std::shared_ptr<CompilerErrorHandler> errorHandler;

  static std::vector<VellumObject> builtinObjects;
  std::vector<VellumObject> importedObjects;
  std::vector<Scope> scopes;
  std::optional<VellumFunction> currentFunction;
};
}  // namespace vellum