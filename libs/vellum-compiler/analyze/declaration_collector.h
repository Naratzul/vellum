#pragma once

#include <string_view>

#include "ast/decl/declaration_visitor.h"
#include "common/types.h"
#include "vellum/vellum_identifier.h"
#include "vellum/vellum_state.h"

namespace vellum {
using common::Map;
using common::Opt;
using common::Set;
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
      const Shared<Resolver>& resolver, std::string_view scriptFileName)
      : errorHandler(errorHandler),
        resolver(resolver),
        scriptFilename(scriptFileName) {}

  void collect(Vec<Unique<ast::Declaration>>& declarations);

  void visitImportDeclaration(ast::ImportDeclaration& declaration) override;
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitStateDeclaration(ast::StateDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
  std::string_view scriptFilename;
  int scriptDeclCount{0};
  VellumState* state{nullptr};
  Map<VellumIdentifier, VellumState> states;

  Map<std::string, VellumIdentifier> normalizedVarNameToOriginal;
  Map<std::string, VellumIdentifier> normalizedPropertyNameToOriginal;

  // state name -> functions
  Map<std::string, Set<std::string>> normalizedFunctionNames;
  // state name -> function name -> original function name
  Map<std::string, Map<std::string, VellumIdentifier>> normalizedFunctionNameToOriginal;

  Set<std::string> normalizedPropertyNames;
  Set<std::string> normalizedVariableNames;
};
}  // namespace vellum