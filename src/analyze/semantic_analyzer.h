#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "vellum/vellum_type.h"

namespace vellum {

class CompilerErrorHandler;

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
  explicit SemanticAnalyzer(std::shared_ptr<CompilerErrorHandler> errorHandler);

  SemanticAnalyzeResult analyze(
      std::vector<std::unique_ptr<ast::Declaration>>&& declarations);

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

  void visitExpressionStatement(ast::ExpressionStatement& statement) override;
  void visitReturnStatement(ast::ReturnStatement& statement) override;
  void visitCallExpression(ast::CallExpression& expr) override;
  void visitGetExpression(ast::GetExpression& expr) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;

  VellumType resolveType(VellumType unresolvedType) const;
  VellumType deduceType(const std::unique_ptr<ast::Expression>& init) const;

  VellumLiteralType resolveValueType(std::string_view rawType) const;
  VellumIdentifier resolveObjectType(std::string_view rawType) const;
};
}  // namespace vellum