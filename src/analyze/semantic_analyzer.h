#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "type_checker.h"
#include "vellum/vellum_type.h"

namespace vellum {
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

class CompilerErrorHandler;
class Resolver;

namespace ast {
class Expression;
class Declaration;
class Statement;
}  // namespace ast

struct SemanticAnalyzeResult {
  Vec<Unique<ast::Declaration>> declarations;
};

class SemanticAnalyzer : public ast::DeclarationVisitor,
                         public ast::StatementVisitor,
                         public ast::ExpressionVisitor {
 public:
  SemanticAnalyzer(Shared<CompilerErrorHandler> errorHandler,
                   Shared<Resolver> resolver, std::string_view scriptFilename);

  SemanticAnalyzeResult analyze(Vec<Unique<ast::Declaration>>&& declarations);

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override {}
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
  void visitCastExpression(ast::CastExpression& expr) override;
  void visitNewArrayExpression(ast::NewArrayExpression& expr) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
  std::string_view scriptFilename;
  TypeChecker checker;
};
}  // namespace vellum