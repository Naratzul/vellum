#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {
void ScriptStatement::accept(StatementVisitor& visitor) {
  visitor.visitScriptStatement(*this);
}

VellumValue VariableDeclaration::getValue() const { 
  if (initializer_) {
    return initializer_->produceValue();
  }
  return VellumValue();
}

void VariableDeclaration::accept(StatementVisitor& visitor) {
  visitor.visitVariableDeclaration(*this);
}

void FunctionDeclaration::accept(StatementVisitor& visitor) {
  visitor.visitFunctionDeclaration(*this);
}
}  // namespace ast
}  // namespace vellum