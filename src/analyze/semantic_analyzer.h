#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "vellum/vellum_type.h"

namespace vellum {

class CompilerErrorHandler;

namespace ast {
class Expression;
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
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;

  VellumType resolveType(VellumType unresolvedType) const;
  VellumType deduceType(const std::unique_ptr<ast::Expression>& init) const;

  VellumValueType resolveValueType(std::string_view rawType) const;
  VellumIdentifier resolveObjectType(std::string_view rawType) const;
};
}  // namespace vellum