#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "ast/expression/expression.h"
#include "ast/expression/expression_visitor.h"
#include "ast/statement/statement_visitor.h"
#include "common/types.h"
#include "lexer/token.h"
#include "type_checker.h"
#include "vellum/vellum_state.h"
#include "vellum/vellum_type.h"

namespace vellum {
using common::Map;
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
  void visitBreakStatement(ast::BreakStatement& statement) override;
  void visitContinueStatement(ast::ContinueStatement& statement) override;
  void visitForStatement(ast::ForStatement& statement) override;

  void visitIdentifierExpression(ast::IdentifierExpression& expr) override;
  void visitCallExpression(ast::CallExpression& expr) override;
  void visitPropertyGetExpression(ast::PropertyGetExpression& expr) override;
  void visitPropertySetExpression(ast::PropertySetExpression& expr) override;
  void visitAssignExpression(ast::AssignExpression& expr) override;
  void visitBinaryExpression(ast::BinaryExpression& expr) override;
  void visitUnaryExpression(ast::UnaryExpression& expr) override;
  void visitCastExpression(ast::CastExpression& expr) override;
  void visitNewArrayExpression(ast::NewArrayExpression& expr) override;
  void visitSelfExpression(ast::SelfExpression& expr) override;
  void visitSuperExpression(ast::SuperExpression& expr) override;
  void visitArrayIndexExpression(ast::ArrayIndexExpression& expr) override;
  void visitArrayIndexSetExpression(
      ast::ArrayIndexSetExpression& expr) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
  std::string_view scriptFilename;
  TypeChecker checker;

  bool inStaticContext{false};
  int loopDepth{0};
  int loopCount{0};

  Opt<VellumState> state;
  Map<VellumIdentifier, VellumFunction> stateFunc;

  bool validateComposedAssignTypes(ast::AssignOperator op,
                                   const VellumType& lhsType,
                                   const VellumType& rhsType,
                                   const Token& location);

  std::string_view mangleLoopVariable(std::string_view name);
};
}  // namespace vellum