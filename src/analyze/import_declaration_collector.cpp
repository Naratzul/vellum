#include "import_declaration_collector.h"

#include "ast/decl/declaration.h"

namespace vellum {
void ImportDeclarationCollector::collect(
    Vec<Unique<ast::Declaration>>& declarations) {
  for (auto& decl : declarations) {
    decl->accept(*this);
  }
}

void ImportDeclarationCollector::visitImportDeclaration(
    ast::ImportDeclaration& declaration) {
  importedNames.insert(VellumIdentifier(declaration.getImportName()));
}

void ImportDeclarationCollector::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  // Recursively collect imports from script members
  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void ImportDeclarationCollector::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {
  // No imports in variable declarations
}

void ImportDeclarationCollector::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {
  // No imports in function declarations
}

void ImportDeclarationCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {
  // No imports in property declarations
}

}  // namespace vellum
