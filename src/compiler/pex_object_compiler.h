#pragma once

#include <memory>

#include "ast/statement_visitor.h"
#include "pex/pex_value.h"
#include "vellum/vellum_value.h"

namespace vellum {

class CompilerErrorHandler;
struct Token;

namespace pex {
class PexFile;
class PexObject;
}  // namespace pex

class PexObjectCompiler : public ast::StatementVisitor {
 public:
  PexObjectCompiler(std::shared_ptr<CompilerErrorHandler> errorHandler,
                    pex::PexFile& file, pex::PexObject& object);

  void visitScriptStatement(ast::ScriptStatement& statement) override;
  void visitVariableDeclaration(ast::VariableDeclaration& statement) override;
  void visitFunctionDeclaration(ast::FunctionDeclaration& statement) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;
  pex::PexObject& object;

  pex::PexValue makeValueFromToken(VellumValue value);
};
}  // namespace vellum