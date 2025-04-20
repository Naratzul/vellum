#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration_visitor.h"
#include "pex/pex_object.h"
#include "pex/pex_value.h"
#include "vellum/vellum_value.h"

namespace vellum {

namespace ast {
class Declaration;
class Statement;
}  // namespace ast

class CompilerErrorHandler;

namespace pex {
class PexFile;
}  // namespace pex

class PexObjectCompiler : public ast::DeclarationVisitor {
 public:
  PexObjectCompiler(std::shared_ptr<CompilerErrorHandler> errorHandler,
                    pex::PexFile& file);

  pex::PexObject compile(
      const std::vector<std::unique_ptr<ast::Declaration>>& statements);

  void visitScriptDeclaration(ast::ScriptDeclaration& statement) override;
  void visitVariableDeclaration(ast::GlobalVariableDeclaration& statement) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& statement) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;
  pex::PexObject object;

  pex::PexValue makeValueFromToken(VellumValue value);
};
}  // namespace vellum