#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {
void ExpressionStatement::accept(StatementVisitor& visitor) {
  visitor.visitExpressionStatement(*this);
}
}  // namespace ast
}  // namespace vellum