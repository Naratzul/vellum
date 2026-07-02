#pragma once

#include "ast/decl/declaration_visitor.h"
#include "common/types.h"

namespace vellum {
using common::Shared;
using common::Unique;
using common::Vec;

class CompilerErrorHandler;

namespace ast {
class Declaration;
}  // namespace ast

class ModifierValidator : public ast::DeclarationVisitor {
 public:
  explicit ModifierValidator(const Shared<CompilerErrorHandler>& errorHandler);

  void validate(Vec<Unique<ast::Declaration>>& declarations);

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override;
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitStateDeclaration(ast::StateDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
};

}  // namespace vellum
