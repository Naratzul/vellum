#include "type_collector.h"

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {

void TypeCollector::collect(Vec<Unique<ast::Declaration>>& declarations) {
  for (auto& decl : declarations) {
    decl->accept(*this);
  }
}

void TypeCollector::visitImportDeclaration(
    ast::ImportDeclaration& declaration) {
  collectTypeFromVellumType(
      VellumType::identifier(declaration.getImportName()));
}

void TypeCollector::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  collectTypeFromVellumType(declaration.getParentScriptName());

  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void TypeCollector::visitStateDeclaration(ast::StateDeclaration& declaration) {
  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void TypeCollector::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  if (auto typeName = declaration.typeName()) {
    collectTypeFromVellumType(typeName.value());
  }
}

void TypeCollector::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  collectTypeFromVellumType(declaration.getReturnTypeName());

  for (const auto& param : declaration.getParameters()) {
    collectTypeFromVellumType(param.type);
  }

  for (const auto& stmt : declaration.getBody()) {
    stmt->accept(*this);
  }
}

void TypeCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  collectTypeFromVellumType(declaration.getTypeName());
}

void TypeCollector::collectTypeFromVellumType(const VellumType& type) {
  if (isLiteralType(type)) {
    return;
  }

  if (type.isArray()) {
    collectTypeFromVellumType(*type.asArraySubtype());
    return;
  }

  if (!type.isResolved()) {
    std::string_view rawType = type.asRawType();
    if (!rawType.empty()) {
      discoveredTypes.insert(VellumIdentifier(rawType));
    }
    return;
  }

  if (type.getState() == vellum::VellumTypeState::Identifier) {
    discoveredTypes.insert(type.asIdentifier());
  }
}

bool TypeCollector::isLiteralType(const VellumType& type) const {
  if (type.getState() == vellum::VellumTypeState::Literal) {
    return true;
  }

  if (!type.isResolved()) {
    std::string_view rawType = type.asRawType();
    return literalTypeFromString(rawType).has_value();
  }

  return false;
}

void TypeCollector::visitExpressionStatement(
    ast::ExpressionStatement& statement) {
  statement.getExpression()->accept(*this);
}

void TypeCollector::visitReturnStatement(ast::ReturnStatement& statement) {
  statement.getExpression()->accept(*this);
}

void TypeCollector::visitIfStatement(ast::IfStatement& statement) {
  statement.getCondition()->accept(*this);

  for (const auto& stmt : statement.getThenBlock()) {
    stmt->accept(*this);
  }

  if (const auto& elseBlock = statement.getElseBlock()) {
    for (const auto& stmt : elseBlock.value()) {
      stmt->accept(*this);
    }
  }
}

void TypeCollector::visitLocalVariableStatement(
    ast::LocalVariableStatement& statement) {
  if (auto type = statement.getType()) {
    collectTypeFromVellumType(type.value());
  }

  if (auto& init = statement.getInitializer()) {
    init->accept(*this);
  }
}

void TypeCollector::visitWhileStatement(ast::WhileStatement& statement) {
  statement.getCondition()->accept(*this);

  for (const auto& stmt : statement.getBody()) {
    stmt->accept(*this);
  }
}

void TypeCollector::visitIdentifierExpression(ast::IdentifierExpression& expr) {
  VellumIdentifier identifier = expr.getIdentifier();
  discoveredTypes.insert(identifier);
}

void TypeCollector::visitCallExpression(ast::CallExpression& expr) {
  expr.getCallee()->accept(*this);

  for (const auto& arg : expr.getArguments()) {
    arg->accept(*this);
  }
}

void TypeCollector::visitPropertyGetExpression(
    ast::PropertyGetExpression& expr) {
  expr.getObject()->accept(*this);
}

void TypeCollector::visitPropertySetExpression(
    ast::PropertySetExpression& expr) {
  expr.getObject()->accept(*this);
  expr.getValue()->accept(*this);
}

void TypeCollector::visitAssignExpression(ast::AssignExpression& expr) {
  expr.getValue()->accept(*this);
}

void TypeCollector::visitBinaryExpression(ast::BinaryExpression& expr) {
  expr.getLeft()->accept(*this);
  expr.getRight()->accept(*this);
}

void TypeCollector::visitUnaryExpression(ast::UnaryExpression& expr) {
  expr.getOperand()->accept(*this);
}

void TypeCollector::visitCastExpression(ast::CastExpression& expr) {
  collectTypeFromVellumType(expr.getTargetType());
  expr.getExpression()->accept(*this);
}

void TypeCollector::visitNewArrayExpression(ast::NewArrayExpression& expr) {
  if (auto subtype = expr.getSubtype()) {
    collectTypeFromVellumType(subtype.value());
  }
}

void TypeCollector::visitSelfExpression(ast::SelfExpression&) {}

void TypeCollector::visitSuperExpression(ast::SuperExpression&) {}

void TypeCollector::visitArrayIndexExpression(ast::ArrayIndexExpression& expr) {
  expr.getArray()->accept(*this);
  expr.getIndex()->accept(*this);
}

void TypeCollector::visitArrayIndexSetExpression(
    ast::ArrayIndexSetExpression& expr) {
  expr.getArray()->accept(*this);
  expr.getIndex()->accept(*this);
  expr.getValue()->accept(*this);
}

}  // namespace vellum
