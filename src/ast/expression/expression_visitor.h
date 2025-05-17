#pragma once

#include "vellum/vellum_value.h"

namespace vellum {

namespace ast {

class CallExpression;
class GetExpression;
class LiteralExpression;
class IdentifierExpression;

class ExpressionVisitor {
 public:
  virtual void visitCallExpression(ast::CallExpression& expr) = 0;
  virtual void visitGetExpression(ast::GetExpression& expr) = 0;
};

class ExpressionVisitorConst {
 public:
  virtual void visitCallExpression(const ast::CallExpression& expr) = 0;
  virtual void visitGetExpression(const ast::GetExpression& expr) = 0;
};

}  // namespace ast
}  // namespace vellum