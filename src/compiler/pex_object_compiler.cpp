#include "pex_object_compiler.h"

#include "ast/statement.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "pex/pex_object.h"

namespace vellum {

PexObjectCompiler::PexObjectCompiler(
    std::shared_ptr<CompilerErrorHandler> errorHandler, pex::PexFile& file,
    pex::PexObject& object)
    : errorHandler(errorHandler), file(file), object(object) {}

void PexObjectCompiler::visitScriptStatement(ast::ScriptStatement& statement) {
  object.setName(file.stringTable().lookup(statement.scriptName()));
  if (statement.parentScriptName().has_value()) {
    object.setParentName(
        file.stringTable().lookup(statement.parentScriptName().value()));
  }
}

}  // namespace vellum