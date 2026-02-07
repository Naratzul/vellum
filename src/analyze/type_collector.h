#pragma once

#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "vellum/vellum_identifier.h"

namespace vellum {
using common::Set;
using common::Unique;
using common::Vec;

namespace ast {
class Declaration;
}  // namespace ast

class VellumType;

class TypeCollector : public ast::DeclarationVisitor,
                      public ast::StatementVisitor,
                      public ast::ExpressionVisitor {
 public:
  void collect(Vec<Unique<ast::Declaration>>& declarations);

  Set<VellumIdentifier> getDiscoveredTypes() const { return discoveredTypes; }

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override;
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitStateDeclaration(ast::StateDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

  void visitExpressionStatement(ast::ExpressionStatement& statement) override;
  void visitReturnStatement(ast::ReturnStatement& statement) override;
  void visitIfStatement(ast::IfStatement& statement) override;
  void visitLocalVariableStatement(
      ast::LocalVariableStatement& statement) override;
  void visitWhileStatement(ast::WhileStatement& statement) override;

  void visitIdentifierExpression(ast::IdentifierExpression& expr) override;
  void visitCallExpression(ast::CallExpression& expr) override;
  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override;
  void visitPropertySetExpression(ast::PropertySetExpression& expr) override;
  void visitAssignExpression(ast::AssignExpression& expr) override;
  void visitBinaryExpression(ast::BinaryExpression& expr) override;
  void visitUnaryExpression(ast::UnaryExpression& expr) override;
  void visitCastExpression(ast::CastExpression& expr) override;
  void visitNewArrayExpression(ast::NewArrayExpression& expr) override;
  void visitSuperExpression(ast::SuperExpression& expr) override;

 private:
  Set<VellumIdentifier> discoveredTypes;

  void collectTypeFromVellumType(const VellumType& type);
  bool isLiteralType(const VellumType& type) const;
};

}  // namespace vellum
