#pragma once

namespace vellum {

namespace ast {

class ExpressionStatement;

class StatementVisitor {
 public:
  virtual void visitExpressionStatement(ExpressionStatement& statement) = 0;
};

}  // namespace ast
}  // namespace vellum