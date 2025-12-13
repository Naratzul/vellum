#include "declaration.h"

#include "ast/expression/expression_visitor.h"
#include "declaration_visitor.h"

namespace vellum {
namespace ast {
void ScriptDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitScriptDeclaration(*this);
}

bool ScriptDeclaration::equals(const Declaration& other_) const {
  auto& other = static_cast<const ScriptDeclaration&>(other_);
  return scriptName() == other.scriptName() &&
         parentScriptName() == other.parentScriptName();
}

VellumValue GlobalVariableDeclaration::getValue() const {
  if (initializer_) {
    return initializer_->asLiteral().getLiteral();
  }
  return VellumValue();
}

void GlobalVariableDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitVariableDeclaration(*this);
}

bool GlobalVariableDeclaration::equals(const Declaration& other_) const {
  auto& other = static_cast<const GlobalVariableDeclaration&>(other_);
  if (name() != other.name() || typeName() != other.typeName()) {
    return false;
  }

  const bool hasInit = initializer() != nullptr;
  if (hasInit != (other.initializer() != nullptr)) {
    return false;
  }

  return !hasInit || initializer()->equals(*other.initializer());
}

void FunctionDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitFunctionDeclaration(*this);
}

bool FunctionDeclaration::equals(const Declaration& other_) const {
  auto& other = static_cast<const FunctionDeclaration&>(other_);
  if (getName() != other.getName()) {
    return false;
  }

  if (getReturnTypeName() != other.getReturnTypeName()) {
    return false;
  }

  if (getParameters() != other.getParameters()) {
    return false;
  }

  if (getBody().size() != other.getBody().size()) {
    return false;
  }

  for (size_t i = 0; i < getBody().size(); i++) {
    if (!getBody()[i]->equals(*other.getBody()[i])) {
      return false;
    }
  }

  return true;
}

void PropertyDeclaration::accept(DeclarationVisitor& visitor) {
  visitor.visitPropertyDeclaration(*this);
}

bool PropertyDeclaration::equals(const Declaration& other_) const {
  auto& other = static_cast<const PropertyDeclaration&>(other_);
  return getName() == other.getName() && getTypeName() == other.getTypeName() &&
         getDocumentationString() == other.getDocumentationString() &&
         getDefaultValue() == other.getDefaultValue() &&
         getGetAccessor() == other.getGetAccessor() &&
         getSetAccessor() == other.getSetAccessor();
}

bool operator==(const Declaration& lhs, const Declaration& rhs) {
  return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
}

bool operator!=(const Declaration& lhs, const Declaration& rhs) {
  return !(lhs == rhs);
}

bool operator==(const FunctionParameter& lhs, const FunctionParameter& rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type;
}

bool operator!=(const FunctionParameter& lhs, const FunctionParameter& rhs) {
  return !(lhs == rhs);
}
}  // namespace ast
}  // namespace vellum