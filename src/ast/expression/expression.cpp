#include "expression.h"

#include "expression_visitor.h"

namespace vellum {
namespace ast {
void LiteralExpression::accept(ExpressionVisitor& visitor) {}
void CallExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitCallExpression(*this);
}
}  // namespace ast
}  // namespace vellum