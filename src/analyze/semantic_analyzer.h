#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "ast/decl/declaration_visitor.h"

namespace vellum {

class CompilerErrorHandler;

namespace ast {
class Declaration;
class Statement;
}  // namespace ast

struct SemanticAnalyzeResult {
  std::vector<std::unique_ptr<ast::Declaration>> declarations;
};

class SemanticAnalyzer : public ast::DeclarationVisitor {
 public:
  explicit SemanticAnalyzer(std::shared_ptr<CompilerErrorHandler> errorHandler);

  SemanticAnalyzeResult analyze(
      std::vector<std::unique_ptr<ast::Declaration>>&& declarations);

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  std::unordered_set<std::string> builtinTypes;
};
}  // namespace vellum