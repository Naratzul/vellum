#pragma once

#include "ast/decl/declaration_visitor.h"
#include "common/types.h"

namespace vellum {
using common::Shared;
using common::Unique;
using common::Vec;

namespace ast {
class Declaration;
}  // namespace ast
class CompilerErrorHandler;
class Resolver;

class DeclarationCollector : public ast::DeclarationVisitor {
 public:
  explicit DeclarationCollector(
      const Shared<CompilerErrorHandler>& errorHandler,
      const Shared<Resolver>& resolver)
      : errorHandler(errorHandler), resolver(resolver) {}

  void collect(Vec<Unique<ast::Declaration>>& declarations);

  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
};
}  // namespace vellum