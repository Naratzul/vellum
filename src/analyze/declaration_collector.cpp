#include "declaration_collector.h"

#include <algorithm>

#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"
#include "compiler/resolver.h"

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

void DeclarationCollector::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  if (resolver->getObject().getType().isResolved()) {
    errorHandler->errorAt(declaration.getScriptNameLocation(),
                          CompilerErrorKind::MultipleScriptsDefinition,
                          "Only 1 Script is allowed per file.");
    return;
  }

  // TODO: self import check
  resolver->setObject(
      VellumObject(VellumType::identifier(declaration.scriptName())));

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

  VellumFunction func(VellumIdentifier(declaration.getName().value()),
                      declaration.getReturnTypeName(), std::move(parameters));

  resolver->addFunction(func);
}

void DeclarationCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  if (!declaration.getTypeName().isResolved()) {
    declaration.getTypeName() =
        resolver->resolveType(declaration.getTypeName());
  }

  resolver->addProperty(VellumProperty(VellumIdentifier(declaration.getName()),
                                       declaration.getTypeName()));
}
}  // namespace vellum