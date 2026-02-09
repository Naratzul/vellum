#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "common/types.h"
#include "pex/pex_object.h"
#include "pex/pex_value.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::Shared;
using common::Unique;
using common::Vec;

namespace ast {
class Declaration;
class Statement;
}  // namespace ast

class CompilerErrorHandler;
class Resolver;

namespace pex {
class PexFile;
}  // namespace pex

class PexObjectCompiler : public ast::DeclarationVisitor {
 public:
  PexObjectCompiler(Shared<CompilerErrorHandler> errorHandler,
                    pex::PexFile& file);

  pex::PexObject compile(const Vec<Unique<ast::Declaration>>& declarations);

  void visitImportDeclaration(ast::ImportDeclaration&) override {}
  void visitScriptDeclaration(ast::ScriptDeclaration& declaration) override;
  void visitStateDeclaration(ast::StateDeclaration& declaration) override;
  void visitVariableDeclaration(
      ast::GlobalVariableDeclaration& declaration) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& declaration) override;
  void visitPropertyDeclaration(ast::PropertyDeclaration& declaration) override;

 private:
  Shared<CompilerErrorHandler> errorHandler;
  Shared<Resolver> resolver;
  pex::PexFile& file;
  pex::PexObject object;
  int currentStateIndex = 0;

  pex::PexValue makeValueFromToken(VellumValue value);
};
}  // namespace vellum