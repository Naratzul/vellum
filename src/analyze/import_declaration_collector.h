#pragma once

#include "ast/decl/declaration_visitor.h"
#include "common/types.h"
#include "vellum/vellum_identifier.h"

namespace vellum {
using common::Set;
using common::Unique;
using common::Vec;

namespace ast {
class Declaration;
}  // namespace ast

class ImportDeclarationCollector : public ast::DeclarationVisitor {
 public:
  void collect(Vec<Unique<ast::Declaration>>& declarations);

  Set<VellumIdentifier> getImportedNames() const { return importedNames; }

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override;
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Set<VellumIdentifier> importedNames;
};
}  // namespace vellum
