#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "ast/statement_visitor.h"

namespace vellum {

class CompilerErrorHandler;

namespace ast {
class Statement;
}  // namespace ast

struct SemanticAnalyzeResult {
  std::vector<std::unique_ptr<ast::Statement>> statements;
};

class SemanticAnalyzer : public ast::StatementVisitor {
 public:
  explicit SemanticAnalyzer(std::shared_ptr<CompilerErrorHandler> errorHandler);

  SemanticAnalyzeResult analyze(
      std::vector<std::unique_ptr<ast::Statement>>&& statements);

  void visitScriptStatement(ast::ScriptStatement& statement) override;
  void visitVariableDeclaration(ast::VariableDeclaration& statement) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& statement) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  std::unordered_set<std::string> builtinTypes;
};
}  // namespace vellum