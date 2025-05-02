#include "expression.h"

#include "expression_visitor.h"

namespace vellum {
namespace ast {
void LiteralExpression::accept(ExpressionVisitor& visitor) {}

bool LiteralExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const LiteralExpression&>(other_);
  return value == other.value;
}

void CallExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitCallExpression(*this);
}

bool CallExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const CallExpression&>(other_);
  return getFunctionName() == other.getFunctionName() &&
         getModuleName() == other.getModuleName() &&
         getArguments() == other.getArguments();
}

bool operator==(const Expression& lhs, const Expression& rhs) {
  return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
}

bool operator!=(const Expression& lhs, const Expression& rhs) {
  return !(lhs == rhs);
}
}  // namespace ast
}  // namespace vellum