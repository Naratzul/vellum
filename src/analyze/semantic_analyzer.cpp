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
    ast::FunctionDeclaration& declaration) {
  for (auto& param : declaration.getParameters()) {
    if (!param.type.isResolved()) {
      param.type = resolveType(param.type);
    }
  }

  if (!declaration.getReturnTypeName().isResolved()) {
    declaration.getReturnTypeName() =
        resolveType(declaration.getReturnTypeName());
  }

  for (auto& statement : declaration.getBody()) {
    statement->accept(*this);
  }
}

void SemanticAnalyzer::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  if (!declaration.getTypeName().isResolved()) {
    declaration.getTypeName() = resolveType(declaration.getTypeName());
  }
}

void SemanticAnalyzer::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void SemanticAnalyzer::visitReturnStatement(ast::ReturnStatement& statement) {
  statement.getExpression()->accept(*this);
}

void SemanticAnalyzer::visitCallExpression(ast::CallExpression& expr) {
  expr.getCallee()->accept(*this);
  const VellumValue callee = expr.getCallee()->produceValue();

  switch (callee.getType()) {
    case VellumValueType::Identifier:
      expr.setFunction(VellumFunction(VellumIdentifier("self"),
                                      callee.asIdentifier(), false));
      break;
    case VellumValueType::PropertyAccess:
      expr.setFunction(VellumFunction(callee.asPropertyAccess().getObject(),
                                      callee.asPropertyAccess().getProperty(),
                                      false));
      break;
    default:
      assert(false && "Unknown callee type");
      break;
  }

  for (auto& arg : expr.getArguments()) {
    arg->accept(*this);
  }
}

void SemanticAnalyzer::visitGetExpression(ast::GetExpression& expr) {}

VellumType SemanticAnalyzer::resolveType(VellumType unresolvedType) const {
  assert(!unresolvedType.isResolved());
  const std::string_view rawType = unresolvedType.asRawType();
  assert(!rawType.empty());

  VellumType type = VellumType::unresolved("");
  if (literalTypeFromString(unresolvedType.asRawType()).has_value()) {
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
  return init->getType();
}

VellumLiteralType SemanticAnalyzer::resolveValueType(
    std::string_view rawType) const {
  return literalTypeFromString(rawType).value();
}

VellumIdentifier SemanticAnalyzer::resolveObjectType(
    std::string_view rawType) const {
  return VellumIdentifier(rawType);
}
}  // namespace vellum