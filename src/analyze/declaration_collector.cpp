#include "declaration_collector.h"

#include <algorithm>
#include <cassert>
#include <string>

#include <format>

#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_property.h"

namespace vellum {
void DeclarationCollector::collect(
    Vec<Unique<ast::Declaration>>& declarations) {
  std::ranges::sort(declarations, [](const auto& lhs, const auto& rhs) {
    return lhs->getOrder() < rhs->getOrder();
  });

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
    return;
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

    VellumIdentifier parentId = parentScriptName.getState() ==
                                        VellumTypeState::Unresolved
                                    ? VellumIdentifier(parentScriptName.asRawType())
                                    : parentScriptName.asIdentifier();
    if (auto resolved = resolver->resolveScriptType(parentId)) {
      resolvedParentType = resolved;
    } else {
      errorHandler->errorAt(location.value(),
                            CompilerErrorKind::UnknownParentScript,
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

void DeclarationCollector::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  Opt<VellumType> annotatedType;
  if (auto type = declaration.typeName()) {
    annotatedType = resolver->resolveType(type.value());
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

  resolver->addVariable(VellumVariable(VellumIdentifier(declaration.name()),
                                       declaration.typeName().value()));
}

void DeclarationCollector::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  Vec<VellumVariable> parameters;
  parameters.reserve(declaration.getParameters().size());

  for (auto& param : declaration.getParameters()) {
    if (!param.type.isResolved()) {
      param.type = resolver->resolveType(param.type);
    }
    parameters.emplace_back(VellumIdentifier(param.name), param.type);
  }

  if (!declaration.getReturnTypeName().isResolved()) {
    declaration.getReturnTypeName() =
        resolver->resolveType(declaration.getReturnTypeName());
  }

  if (auto parentFunc =
          resolver->resolveParentFunction(VellumIdentifier(declaration.getName().value()))) {
    if (parentFunc->getArity() != static_cast<int>(declaration.getParameters().size()) ||
        parentFunc->getReturnType() != declaration.getReturnTypeName() ||
        parentFunc->isStatic() != declaration.isStatic()) {
      errorHandler->errorAt(Token(), CompilerErrorKind::OverrideSignatureMismatch,
                           "Override of '{}' must match parent signature (parameters and return type).",
                           declaration.getName().value());
      return;
    }
    for (size_t i = 0; i < declaration.getParameters().size(); ++i) {
      if (parentFunc->getParameters()[i].getType() !=
          declaration.getParameters()[i].type) {
        errorHandler->errorAt(Token(), CompilerErrorKind::OverrideSignatureMismatch,
                             "Override of '{}' must match parent signature (parameters and return type).",
                             declaration.getName().value());
        return;
      }
    }
  }

  VellumFunction func(VellumIdentifier(declaration.getName().value()),
                      declaration.getReturnTypeName(), std::move(parameters),
                      declaration.isStatic());

  resolver->addFunction(func);
}

void DeclarationCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  if (!declaration.getTypeName().isResolved()) {
    declaration.getTypeName() =
        resolver->resolveType(declaration.getTypeName());
  }

  if (resolver->resolveParentProperty(VellumIdentifier(declaration.getName()))) {
    errorHandler->errorAt(
        Token(), CompilerErrorKind::CannotOverrideProperty,
        "Cannot override property '{}'; properties in a parent script cannot "
        "be overridden in the child.",
        declaration.getName());
    return;
  }

  resolver->addProperty(VellumProperty(VellumIdentifier(declaration.getName()),
                                       declaration.getTypeName(),
                                       declaration.isReadonly()));
}
}  // namespace vellum