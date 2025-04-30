#pragma once

namespace vellum {

namespace ast {

class ExpressionStatement;
class ReturnStatement;

class StatementVisitor {
 public:
  virtual void visitExpressionStatement(ExpressionStatement& statement) = 0;
  virtual void visitReturnStatement(ReturnStatement& statement) = 0;
};

}  // namespace ast
}  // namespace vellum