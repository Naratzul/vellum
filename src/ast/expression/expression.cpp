#include "expression.h"

#include "expression_visitor.h"

namespace vellum {
namespace ast {

bool LiteralExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const LiteralExpression&>(other_);
  return literal == other.literal;
}

bool IdentifierExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const IdentifierExpression&>(other_);
  return identifier == other.identifier;
}

void CallExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitCallExpression(*this);
}

void CallExpression::accept(ExpressionVisitorConst& visitor) const {
  visitor.visitCallExpression(*this);
}

bool CallExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const CallExpression&>(other_);
  return getCallee()->equals(*other.getCallee()) &&
         getArguments() == other.getArguments();
}

bool GetExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const GetExpression&>(other_);
  return getObject()->equals(*other.getObject()) &&
         getProperty() == other.getProperty();
}

VellumValue GetExpression::produceValue() const {
  const VellumValue objectValue = object->produceValue();
  assert(objectValue.getType() == VellumValueType::Identifier);
  return VellumPropertyAccess(objectValue.asIdentifier(), property);
}

bool operator==(const Expression& lhs, const Expression& rhs) {
  return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
}

bool operator!=(const Expression& lhs, const Expression& rhs) {
  return !(lhs == rhs);
}
}  // namespace ast
}  // namespace vellum