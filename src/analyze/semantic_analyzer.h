#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "vellum/vellum_type.h"

namespace vellum {

class CompilerErrorHandler;
class Resolver;

namespace ast {
class Expression;
class Declaration;
class Statement;
}  // namespace ast

struct SemanticAnalyzeResult {
  std::vector<std::unique_ptr<ast::Declaration>> declarations;
};

class SemanticAnalyzer : public ast::DeclarationVisitor,
                         public ast::StatementVisitor,
                         public ast::ExpressionVisitor {
 public:
  SemanticAnalyzer(std::shared_ptr<CompilerErrorHandler> errorHandler,
                   std::shared_ptr<Resolver> resolver);

  SemanticAnalyzeResult analyze(
      std::vector<std::unique_ptr<ast::Declaration>>&& declarations);

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
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

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  std::shared_ptr<Resolver> resolver;

  VellumType resolveType(VellumType unresolvedType) const;
  VellumType deduceType(const std::unique_ptr<ast::Expression>& init) const;

  VellumLiteralType resolveValueType(std::string_view rawType) const;
  VellumIdentifier resolveObjectType(std::string_view rawType) const;
};
}  // namespace vellum