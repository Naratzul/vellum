#pragma once

#include <memory>

#include "ast/statement_visitor.h"

namespace vellum {

class CompilerErrorHandler;

namespace pex {
class PexFile;
class PexObject;
}  // namespace pex

class PexObjectCompiler : public ast::StatementVisitor {
 public:
  PexObjectCompiler(std::shared_ptr<CompilerErrorHandler> errorHandler,
                    pex::PexFile& file, pex::PexObject& object);

  void visitScriptStatement(ast::ScriptStatement& statement) override;

 private:
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  pex::PexFile& file;
  pex::PexObject& object;
};
}  // namespace vellum