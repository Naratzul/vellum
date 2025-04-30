#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {
void ExpressionStatement::accept(StatementVisitor& visitor) {
  visitor.visitExpressionStatement(*this);
}

void ReturnStatement::accept(StatementVisitor& visitor) {
  visitor.visitReturnStatement(*this);
}
}  // namespace ast
}  // namespace vellum