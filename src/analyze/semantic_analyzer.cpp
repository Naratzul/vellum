#include "semantic_analyzer.h"

#include <cassert>

#include "ast/statement.h"
#include "compiler/compiler_error_handler.h"

namespace vellum {

SemanticAnalyzer::SemanticAnalyzer(
    std::shared_ptr<CompilerErrorHandler> errorHandler)
    : errorHandler(errorHandler) {
  builtinTypes = {"Int", "Float", "Bool", "String"};
}

SemanticAnalyzeResult SemanticAnalyzer::analyze(
    std::vector<std::unique_ptr<ast::Statement>>&& statements) {
  for (auto& statement : statements) {
    statement->accept(*this);
  }

  return SemanticAnalyzeResult(std::move(statements));
}

void SemanticAnalyzer::visitScriptStatement(ast::ScriptStatement& statement) {}

void SemanticAnalyzer::visitVariableDeclaration(
    ast::VariableDeclaration& statement) {
  const VellumValue value = statement.getValue();
  if (statement.typeName().has_value()) {
    if (!builtinTypes.contains(std::string(statement.typeName().value()))) {
      errorHandler->errorAt(Token(), "Unknown variable type");
    }
  } else {
    assert(statement.initializer());
    statement.typeName() = valueTypeToString(value.getType());
  }

  if (statement.initializer()) {
    if (statement.typeName() != valueTypeToString(value.getType())) {
      errorHandler->errorAt(Token(), "Variable type mismatch.");
    }
  }
}
}  // namespace vellum