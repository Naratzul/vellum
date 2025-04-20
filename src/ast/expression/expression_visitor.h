#pragma once

namespace vellum {

namespace ast {

class CallExpression;

class ExpressionVisitor {
 public:
  virtual void visitCallExpression(ast::CallExpression& expr) = 0;
};

}  // namespace ast
}  // namespace vellum