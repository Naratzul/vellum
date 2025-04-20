#include "semantic_analyzer.h"

#include <cassert>

#include "ast/expression/expression.h"
#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"

namespace vellum {

SemanticAnalyzer::SemanticAnalyzer(
    std::shared_ptr<CompilerErrorHandler> errorHandler)
    : errorHandler(errorHandler) {
  builtinTypes = {"Int", "Float", "Bool", "String"};
}

SemanticAnalyzeResult SemanticAnalyzer::analyze(
    std::vector<std::unique_ptr<ast::Declaration>>&& declarations) {
  for (auto& declaration : declarations) {
    declaration->accept(*this);
  }

  return SemanticAnalyzeResult{std::move(declarations)};
}

void SemanticAnalyzer::visitScriptDeclaration(
    ast::ScriptDeclaration& statement) {}

void SemanticAnalyzer::visitVariableDeclaration(
    ast::GlobalVariableDeclaration& statement) {
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

void SemanticAnalyzer::visitFunctionDeclaration(
    ast::FunctionDeclaration& statement) {}
}  // namespace vellum