#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {
void ScriptStatement::accept(StatementVisitor& visitor) {
  visitor.visitScriptStatement(*this);
}
}  // namespace ast
}  // namespace vellum