#include "declaration_collector.h"

#include <algorithm>

#include "ast/decl/declaration.h"

namespace vellum {
void DeclarationCollector::collect(
    Vec<Unique<ast::Declaration>>& declarations) {
  std::ranges::sort(declarations, [](const auto& lhs, const auto& rhs) {
    return lhs->getOrder() < rhs->getOrder();
  });

  for (auto& decl : declarations) {
    decl->accept(*this);
  }
}

void DeclarationCollector::visitScriptDeclaration(
    ast::ScriptDeclaration& declaration) {
  for (const auto& memberDecl : declaration.getMemberDecls()) {
    memberDecl->accept(*this);
  }
}

void DeclarationCollector::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& declaration) {}

void DeclarationCollector::visitFunctionDeclaration(
    ast::FunctionDeclaration& declaration) {}

void DeclarationCollector::visitPropertyDeclaration(
    ast::PropertyDeclaration& declaration) {}
}  // namespace vellum