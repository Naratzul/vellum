#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {
void ExpressionStatement::accept(StatementVisitor& visitor) {
  visitor.visitExpressionStatement(*this);
}

bool ExpressionStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const ExpressionStatement&>(other_);
  return getExpression()->equals(*other.getExpression());
}

void ReturnStatement::accept(StatementVisitor& visitor) {
  visitor.visitReturnStatement(*this);
}

bool ReturnStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const ReturnStatement&>(other_);
  return getExpression()->equals(*other.getExpression());
}

bool operator==(const Statement& lhs, const Statement& rhs) {
  return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
}

bool operator!=(const Statement& lhs, const Statement& rhs) {
  return !(lhs == rhs);
}
}  // namespace ast
}  // namespace vellum