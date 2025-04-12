#include "pex_object_compiler.h"

#include <charconv>

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

  object.setDocumentationString(file.stringTable().lookup(""));
  object.setAutoStateName(file.stringTable().lookup(""));
}

void PexObjectCompiler::visitVariableDeclaration(
    ast::VariableDeclaration& statement) {
  const pex::PexString name = file.stringTable().lookup(statement.name());

  assert(statement.typeName().has_value());
  const pex::PexString typeName =
      file.stringTable().lookup(statement.typeName().value());

  const pex::PexValue defaultValue = makeValueFromToken(statement.getValue());
  object.getVariables().emplace_back(name, typeName, defaultValue);
}

pex::PexValue PexObjectCompiler::makeValueFromToken(VellumValue value) {
  switch (value.getType()) {
    case VellumValueType::String: {
      const pex::PexString pexValue =
          file.stringTable().lookup(value.asString());
      return pex::PexValue(pexValue);
    }
    case VellumValueType::Int:
      return pex::PexValue(value.asInt());
    case VellumValueType::Float:
      return pex::PexValue(value.asFloat());
    case VellumValueType::Bool:
      return pex::PexValue(value.asBool());
    case VellumValueType::Nil:
      return pex::PexValue();
  }

  errorHandler->errorAt(Token(), "Unexpected variable initializer type.");
  return pex::PexValue();
}

}  // namespace vellum