#include "declaration_collector.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <optional>
#include <string>

#include "ast/decl/declaration.h"
#include "common/string_utils.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_property.h"
#include "vellum/vellum_state.h"

namespace vellum {
void DeclarationCollector::collect(
    Vec<Unique<ast::Declaration>>& declarations) {
  for (auto& decl : declarations) {
    decl->accept(*this);
  }
}

void DeclarationCollector::visitImportDeclaration(
    ast::ImportDeclaration& declaration) {
  std::string_view importName = declaration.getImportName();

  bool isLiteralType = literalTypeFromString(importName).has_value();

  if (isLiteralType) {
    errorHandler->errorAt(
        declaration.getImportNameLocation(),
        CompilerErrorKind::CannotImportLiteralType,
        "Cannot import literal type '{}'. Literal types (Int, Float, String, "
        "Bool) cannot be imported.",
        importName);
    return;
  }

  resolver->importObject(VellumIdentifier(importName));
}

void DeclarationCollector::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  if (scriptDeclCount > 0) {
    errorHandler->errorAt(declaration.getScriptNameLocation(),
                          CompilerErrorKind::MultipleScriptsDefinition,
                          "Only 1 Script is allowed per file.");
    return;
  }

  auto scriptName = declaration.getScriptName();
  if (scriptName.toString() != scriptFilename) {
    errorHandler->errorAt(declaration.getScriptNameLocation(),
                          CompilerErrorKind::FilenameMismatch,
                          "Filename '{}' doesn't match scriptname '{}'.",
                          scriptFilename, scriptName.toString());
  }

  Opt<VellumType> resolvedParentType;
  if (auto location = declaration.getParentScriptNameLocation()) {
    auto parentScriptName = declaration.getParentScriptName();
    if (parentScriptName.getState() == VellumTypeState::Literal) {
      errorHandler->errorAt(location.value(),
                            CompilerErrorKind::CannotExtendFromLiteralType,
                            "Cannot extend from literal type '{}'. Scripts "
                            "can only extend from other scripts, not literal "
                            "types (Int, Float, String, Bool).",
                            parentScriptName.toString());
      return;
    }

    if (parentScriptName == scriptName) {
      errorHandler->errorAt(location.value(),
                            CompilerErrorKind::CannotExtendOutself,
                            "Cannot extend ourself.");
      return;
    }

    VellumIdentifier parentId =
        parentScriptName.getState() == VellumTypeState::Unresolved
            ? VellumIdentifier(parentScriptName.asRawType())
            : parentScriptName.asIdentifier();
    if (auto resolved = resolver->resolveScriptType(parentId)) {
      resolvedParentType = resolved;
    } else {
      errorHandler->errorAt(
          location.value(), CompilerErrorKind::UnknownParentScript,
          "Unknown parent script '{}'.", parentScriptName.toString());
      return;
    }
  }

  scriptDeclCount++;

  resolver->setObject(VellumObject(scriptName));
  if (resolvedParentType) {
    resolver->setParentType(resolvedParentType);
  }

  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void DeclarationCollector::visitStateDeclaration(
    ast::StateDeclaration& declaration) {
  if (declaration.getIsAuto()) {
    autoStateCount++;
  }

  if (autoStateCount > 1) {
    errorHandler->errorAt(declaration.getStateNameLocation(),
                          CompilerErrorKind::MultipleAutoStates,
                          "Only one auto state is allowed.");
  }

  // TODO: add name case collision check
  state = VellumState(VellumIdentifier(declaration.getStateName()));

  auto& members = declaration.getMemberDecls();
  for (const auto& member : members) {
    member->accept(*this);
  }

  std::erase_if(members, [](const auto& decl) {
    return decl->getOrder() != ast::DeclarationOrder::Function;
  });

  resolver->addState(state.value());
  state = std::nullopt;
}

void DeclarationCollector::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  if (state.has_value()) {
    errorHandler->errorAt(
        declaration.getNameLocation().value_or(Token()),
        CompilerErrorKind::ExpectDeclaration,
        "Variable '{}' is not allowed inside states. Only functions and events "
        "are allowed in state declarations.",
        declaration.name());
    return;
  }

  Opt<VellumType> annotatedType;
  if (auto type = declaration.typeName()) {
    Token typeLocation = declaration.getTypeLocation().value_or(Token());
    annotatedType = resolver->resolveType(type.value(), typeLocation);
    declaration.typeName() = annotatedType;
  }

  Opt<VellumType> deducedType;
  if (declaration.initializer()) {
    deducedType = declaration.initializer()->getType();
    if (!annotatedType) {
      declaration.typeName() = deducedType;
    }
  }

  if (annotatedType && deducedType) {
    if (annotatedType != deducedType) {
      std::string gotType;
      if (deducedType->isResolved()) {
        gotType = std::format(", got '{}'.", deducedType->toString());
      }

      errorHandler->errorAt(
          declaration.initializer()->getLocation(),
          CompilerErrorKind::VariableTypeMismatch,
          std::format("Variable type mismatch: expected '{}'{}.",
                      annotatedType->toString(), gotType));
    }
  }

  auto varName = VellumIdentifier(declaration.name());
  std::string normalized =
      std::string(common::normalizeToLower(varName.toString()));

  if (normalizedVariableNames.contains(normalized)) {
    auto it = normalizedToOriginal.find(normalized);
    if (it != normalizedToOriginal.end() && it->second != varName) {
      Token varLocation = declaration.getNameLocation().value_or(Token());
      errorHandler->errorAt(
          varLocation, CompilerErrorKind::CaseConflict,
          "Variable '{}' conflicts with '{}'. When compiled to PEX (Papyrus "
          "format), these names are case-insensitive and would create a "
          "duplicate.",
          varName.toString(), it->second.toString());
      return;
    }
  }

  normalizedVariableNames.insert(normalized);
  normalizedToOriginal.emplace(normalized, varName);

  resolver->addVariable(
      VellumVariable(varName, declaration.typeName().value()));
}

void DeclarationCollector::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  Vec<VellumVariable> parameters;
  parameters.reserve(declaration.getParameters().size());

  bool seenOptional = false;
  for (auto& param : declaration.getParameters()) {
    if (!param.type.isResolved()) {
      param.type = resolver->resolveType(param.type, param.typeLocation);
    }

    if (param.defaultValue.has_value()) {
      seenOptional = true;
      const auto& lit = *param.defaultValue;
      const auto& paramType = param.type;
      bool typeOk = false;
      switch (lit.getType()) {
        case VellumLiteralType::Int:
          typeOk = paramType.isInt();
          break;
        case VellumLiteralType::Float:
          typeOk = paramType.isFloat();
          break;
        case VellumLiteralType::Bool:
          typeOk = paramType.isBool();
          break;
        case VellumLiteralType::String:
          typeOk = paramType.isString();
          break;
        case VellumLiteralType::None:
          typeOk = paramType.isArray() ||
                   paramType.getState() == VellumTypeState::Identifier;
          break;
      }
      if (!typeOk) {
        errorHandler->errorAt(
            param.typeLocation, CompilerErrorKind::VariableTypeMismatch,
            "Default value type does not match parameter type '{}'.",
            paramType.toString());
        return;
      }
    } else if (seenOptional) {
      errorHandler->errorAt(
          param.nameLocation, CompilerErrorKind::ExpectParamName,
          "Required parameter cannot follow optional parameter.");
      return;
    }

    parameters.emplace_back(VellumIdentifier(param.name), param.type,
                            param.defaultValue, param.nameLocation);
  }

  if (!declaration.getReturnTypeName().isResolved()) {
    Token returnTypeLocation =
        declaration.getReturnTypeLocation().value_or(Token());
    declaration.getReturnTypeName() = resolver->resolveType(
        declaration.getReturnTypeName(), returnTypeLocation);
  }

  auto funcName = VellumIdentifier(declaration.getName().value());
  std::string normalized =
      std::string(common::normalizeToLower(funcName.toString()));
  Token funcLocation = declaration.getNameLocation().value_or(Token());

  if (normalizedFunctionNames.contains(normalized)) {
    auto it = normalizedToOriginal.find(normalized);
    if (it != normalizedToOriginal.end() && it->second != funcName) {
      errorHandler->errorAt(
          funcLocation, CompilerErrorKind::CaseConflict,
          "Function '{}' conflicts with '{}'. When compiled to PEX (Papyrus "
          "format), these names are case-insensitive and would create a "
          "duplicate.",
          funcName.toString(), it->second.toString());
      return;
    }
  }

  if (auto parentFunc = resolver->resolveParentFunction(funcName)) {
    if (parentFunc->getArity() !=
            static_cast<int>(declaration.getParameters().size()) ||
        parentFunc->getReturnType() != declaration.getReturnTypeName() ||
        parentFunc->isStatic() != declaration.isStatic()) {
      errorHandler->errorAt(funcLocation,
                            CompilerErrorKind::OverrideSignatureMismatch,
                            "Override of '{}' must match parent signature "
                            "(parameters and return type).",
                            declaration.getName().value());
      return;
    }
    for (size_t i = 0; i < declaration.getParameters().size(); ++i) {
      if (parentFunc->getParameters()[i].getType() !=
          declaration.getParameters()[i].type) {
        errorHandler->errorAt(funcLocation,
                              CompilerErrorKind::OverrideSignatureMismatch,
                              "Override of '{}' must match parent signature "
                              "(parameters and return type).",
                              declaration.getName().value());
        return;
      }
      if (parentFunc->getParameters()[i].getDefaultValue() !=
          declaration.getParameters()[i].defaultValue) {
        errorHandler->errorAt(funcLocation,
                              CompilerErrorKind::OverrideSignatureMismatch,
                              "Override of '{}' must match parent default value "
                              "for parameter '{}'.",
                              declaration.getName().value(),
                              declaration.getParameters()[i].name);
        return;
      }
    }
  }

  normalizedFunctionNames.insert(normalized);
  normalizedToOriginal.emplace(normalized, funcName);

  VellumFunction func(funcName, declaration.getReturnTypeName(),
                      std::move(parameters), declaration.isStatic());

  if (state.has_value()) {
    state->addFunction(func);
  } else {
    resolver->addFunction(func);
  }
}

void DeclarationCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  if (state.has_value()) {
    errorHandler->errorAt(
        declaration.getNameLocation(), CompilerErrorKind::ExpectDeclaration,
        "Property '{}' is not allowed inside states. Only functions and events "
        "are allowed in state declarations.",
        declaration.getName());
    return;
  }

  if (!declaration.getTypeName().isResolved()) {
    declaration.getTypeName() = resolver->resolveType(
        declaration.getTypeName(), declaration.getTypeLocation());
  }

  auto propName = VellumIdentifier(declaration.getName());
  std::string normalized =
      std::string(common::normalizeToLower(propName.toString()));
  Token propLocation = declaration.getNameLocation();

  if (normalizedPropertyNames.contains(normalized)) {
    auto it = normalizedToOriginal.find(normalized);
    if (it != normalizedToOriginal.end() && it->second != propName) {
      errorHandler->errorAt(
          propLocation, CompilerErrorKind::CaseConflict,
          "Property '{}' conflicts with '{}'. When compiled to PEX (Papyrus "
          "format), these names are case-insensitive and would create a "
          "duplicate.",
          propName.toString(), it->second.toString());
      return;
    }
  }

  if (resolver->resolveParentProperty(propName)) {
    errorHandler->errorAt(
        propLocation, CompilerErrorKind::CannotOverrideProperty,
        "Cannot override property '{}'; properties in a parent script cannot "
        "be overridden in the child.",
        declaration.getName());
    return;
  }

  normalizedPropertyNames.insert(normalized);
  normalizedToOriginal.emplace(normalized, propName);

  resolver->addProperty(VellumProperty(propName, declaration.getTypeName(),
                                       declaration.isReadonly()));
}
}  // namespace vellum