#pragma once

#include <optional>

#include "common/types.h"
#include "lexer/token.h"
#include "scope.h"
#include "vellum/vellum_object.h"
#include "vellum/vellum_state.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Opt;
using common::Shared;
using common::Vec;

class ImportLibrary;
class CompilerErrorHandler;
class BuiltinFunctions;

class Resolver {
 public:
  static void addBultinObject(VellumObject object) {
    builtinObjects.push_back(std::move(object));
  }

  Resolver(VellumObject object,
           const Shared<CompilerErrorHandler>& errorHandler,
           const Shared<ImportLibrary>& importLibrary,
           const Shared<BuiltinFunctions>& builtinFunctions)
      : object(std::move(object)),
        errorHandler(errorHandler),
        importLibrary(importLibrary),
        builtinFunctions(builtinFunctions) {}

  void setObject(const VellumObject& obj) { object = obj; }
  const VellumObject& getObject() const { return object; }

  void setParentType(Opt<VellumType> type) { parentType = std::move(type); }
  Opt<VellumType> getParentType() const { return parentType; }

  void addFunction(VellumFunction function) {
    object.addFunction(std::move(function));
  }

  void addState(VellumState state) { object.addState(std::move(state)); }

  Opt<VellumState> getState(VellumIdentifier name) const {
    return object.getState(name);
  }

  Opt<VellumFunction> getFunction(VellumIdentifier id) const {
    return object.getFunction(id);
  }

  void addProperty(VellumProperty property) {
    object.addProperty(std::move(property));
  }

  Opt<VellumProperty> getProperty(VellumIdentifier id) const {
    return object.getProperty(id);
  }

  void addVariable(VellumVariable variable) {
    object.addVariable(std::move(variable));
  }

  void importObject(VellumIdentifier importedObject) {
    importedObjects.push_back(std::move(importedObject));
  }

  const Opt<VellumFunction>& getCurrentFunction() { return currentFunction; }

  void startFunction(const VellumFunction& func);
  void endFunction();

  void pushScope();
  void popScope();

  void pushLocalVar(const VellumVariable& var, Token location);
  void popLocalVar();
  void popLocalVar(int count);

  Opt<VellumValue> resolveIdentifier(VellumIdentifier identifier) const;

  Opt<VellumValue> resolveProperty(VellumType type,
                                   VellumIdentifier member) const;

  Opt<VellumFunction> resolveFunction(VellumType type,
                                      VellumIdentifier function) const;

  Opt<VellumFunction> resolveParentFunction(VellumIdentifier function) const;

  Opt<VellumValue> resolveParentProperty(VellumIdentifier member) const;

  Opt<VellumVariable> resolveVariable(VellumIdentifier name) const;

  Opt<VellumType> resolveScriptType(VellumIdentifier identifier) const;

  VellumType resolveType(VellumType unresolvedType, Token location) const;

 private:
  VellumObject object;
  Shared<CompilerErrorHandler> errorHandler;
  Shared<ImportLibrary> importLibrary;
  Shared<BuiltinFunctions> builtinFunctions;
  Opt<VellumType> parentType;

  static Vec<VellumObject> builtinObjects;
  Vec<VellumIdentifier> importedObjects;
  Vec<Scope> scopes;
  Opt<VellumFunction> currentFunction;

  VellumLiteralType resolveValueType(std::string_view rawType) const;
  VellumType resolveObjectType(std::string_view rawType, Token location) const;
};
}  // namespace vellum