#include "semantic_analyzer.h"

#include <cassert>
#include <format>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "compiler/compiler_error_handler.h"

namespace vellum {

SemanticAnalyzer::SemanticAnalyzer(
    std::shared_ptr<CompilerErrorHandler> errorHandler)
    : errorHandler(errorHandler) {}

SemanticAnalyzeResult SemanticAnalyzer::analyze(
    std::vector<std::unique_ptr<ast::Declaration>>&& declarations) {
  for (auto& declaration : declarations) {
    declaration->accept(*this);
  }

  return SemanticAnalyzeResult{std::move(declarations)};
}

void SemanticAnalyzer::visitScriptDeclaration(
    ast::ScriptDeclaration& statement) {}

void SemanticAnalyzer::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& statement) {
  std::optional<VellumType> annotatedType;
  if (auto type = statement.typeName()) {
    annotatedType = resolveType(type.value());
    statement.typeName() = annotatedType;
  }

  std::optional<VellumType> deducedType;
  if (statement.initializer()) {
    deducedType = deduceType(statement.initializer());
    if (!annotatedType) {
      statement.typeName() = deducedType;
    }
  }

  if (annotatedType && deducedType) {
    if (annotatedType.value() != deducedType.value()) {
      errorHandler->errorAt(
          Token(),
          std::format("Variable type mismatch: got {}, expected {}.",
                      annotatedType->toString(), deducedType->toString()));
    }
  }
}

void SemanticAnalyzer::visitFunctionDeclaration(
    ast::FunctionDeclaration& statement) {}

void SemanticAnalyzer::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {}

VellumType SemanticAnalyzer::resolveType(VellumType unresolvedType) const {
  assert(!unresolvedType.isResolved());
  const std::string_view rawType = unresolvedType.asRawType();
  assert(!rawType.empty());

  VellumType type = VellumType::unresolved("");
  if (typeFromString(unresolvedType.asRawType()).has_value()) {
    type = VellumType::literal(resolveValueType(unresolvedType.asRawType()));
  } else {
    type =
        VellumType::identifier(resolveObjectType(unresolvedType.asRawType()));
  }

  return type;
}

VellumType SemanticAnalyzer::deduceType(
    const std::unique_ptr<ast::Expression>& init) const {
  assert(init);
  return VellumType::literal(init->produceValue().getType());
}

VellumValueType SemanticAnalyzer::resolveValueType(
    std::string_view rawType) const {
  return typeFromString(rawType).value();
}
VellumIdentifier SemanticAnalyzer::resolveObjectType(
    std::string_view rawType) const {
  return VellumIdentifier(rawType);
}
}  // namespace vellum