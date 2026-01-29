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

class VellumType;

class TypeCollector : public ast::DeclarationVisitor {
 public:
  void collect(Vec<Unique<ast::Declaration>>& declarations);

  Set<VellumIdentifier> getDiscoveredTypes() const { return discoveredTypes; }

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override;
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Set<VellumIdentifier> discoveredTypes;

  void collectTypeFromVellumType(const VellumType& type);
  bool isLiteralType(const VellumType& type) const;
};

}  // namespace vellum
