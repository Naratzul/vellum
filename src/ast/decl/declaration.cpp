#include "declaration.h"

#include "declaration_visitor.h"

namespace vellum {
namespace ast {
void ScriptDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitScriptDeclaration(*this);
}

VellumValue GlobalVariableDeclaration::getValue() const {
  if (initializer_) {
    return initializer_->produceValue();
  }
  return VellumValue();
}

void GlobalVariableDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitVariableDeclaration(*this);
}

void FunctionDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitFunctionDeclaration(*this);
}

void PropertyDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitPropertyDeclaration(*this);
}
}  // namespace ast
}  // namespace vellum