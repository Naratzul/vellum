#pragma once

#include <optional>

#include "common/types.h"
#include "compiler_error_handler.h"
#include "scope.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Opt;
using common::Shared;
using common::Vec;

class Resolver {
 public:
  static void addBultinObject(VellumObject object) {
    builtinObjects.push_back(std::move(object));
  }

  Resolver(VellumObject object, Shared<CompilerErrorHandler> errorHandler)
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

  const Opt<VellumFunction>& getCurrentFunction() { return currentFunction; }

  void startFunction(const VellumFunction& func);
  void endFunction();

  void pushScope();
  void popScope();

  void pushLocalVar(const VellumVariable& var);
  void popLocalVar();
  void popLocalVar(int count);

  Opt<VellumValue> resolveIdentifier(VellumIdentifier identifier) const;

  Opt<VellumValue> resolveProperty(VellumType type,
                                   VellumIdentifier member) const;

  Opt<VellumFunction> resolveFunction(VellumType type,
                                      VellumIdentifier function) const;

  Opt<VellumVariable> resolveVariable(VellumIdentifier name) const;

  Opt<VellumType> resolveScriptType(VellumIdentifier identifier) const;

 private:
  VellumObject object;
  Shared<CompilerErrorHandler> errorHandler;

  static Vec<VellumObject> builtinObjects;
  Vec<VellumObject> importedObjects;
  Vec<Scope> scopes;
  Opt<VellumFunction> currentFunction;
};
}  // namespace vellum