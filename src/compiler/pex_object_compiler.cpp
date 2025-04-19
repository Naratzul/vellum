#include "pex_object_compiler.h"

#include "ast/statement.h"
#include "compiler_error_handler.h"
#include "pex/pex_file.h"
#include "pex/pex_function.h"
#include "pex/pex_object.h"
#include "pex/pex_state.h"

namespace vellum {

PexObjectCompiler::PexObjectCompiler(
    std::shared_ptr<CompilerErrorHandler> errorHandler, pex::PexFile& file,
    pex::PexObject& object)
    : errorHandler(errorHandler), file(file), object(object) {
  assert(object.getStates().empty());
  pex::PexState rootState(file.getString(""));
  object.getStates().push_back(std::move(rootState));
}

void PexObjectCompiler::visitScriptStatement(ast::ScriptStatement& statement) {
  object.setName(file.getString(statement.scriptName()));
  if (statement.parentScriptName().has_value()) {
    object.setParentName(file.getString(statement.parentScriptName().value()));
  }

  object.setDocumentationString(file.getString(""));
  object.setAutoStateName(file.getString(""));
}

void PexObjectCompiler::visitVariableDeclaration(
    ast::VariableDeclaration& statement) {
  const pex::PexString name = file.getString(statement.name());

  assert(statement.typeName().has_value());
  const pex::PexString typeName = file.getString(statement.typeName().value());

  const pex::PexValue defaultValue = makeValueFromToken(statement.getValue());
  object.getVariables().emplace_back(name, typeName, defaultValue);
}

void PexObjectCompiler::visitFunctionDeclaration(
    ast::FunctionDeclaration& statement) {
  for (const auto& stmt : statement.body()) {
    stmt->accept(*this);
  }
}

pex::PexValue PexObjectCompiler::makeValueFromToken(VellumValue value) {
  switch (value.getType()) {
    case VellumValueType::String: {
      const pex::PexString pexValue = file.getString(value.asString());
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