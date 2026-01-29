#pragma once

#include <string_view>

#include "ast/decl/declaration_visitor.h"
#include "common/types.h"
#include "vellum/vellum_identifier.h"

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
      const Shared<Resolver>& resolver, std::string_view scriptFileName,
      bool isPapyrus = false)
      : errorHandler(errorHandler),
        resolver(resolver),
        scriptFilename(scriptFileName),
        isPapyrus(isPapyrus) {}

  void collect(Vec<Unique<ast::Declaration>>& declarations);

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override;
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
  std::string_view scriptFilename;
  int scriptDeclCount = 0;
  bool isPapyrus;
};
}  // namespace vellum