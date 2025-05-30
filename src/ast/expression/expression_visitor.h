#pragma once

#include "pex/pex_value.h"

namespace vellum {

namespace ast {

class CallExpression;
class GetExpression;
class IdentifierExpression;
class LiteralExpression;
class IdentifierExpression;
class AssignExpression;

class ExpressionVisitor {
 public:
  virtual void visitIdentifierExpression(ast::IdentifierExpression& expr) = 0;
  virtual void visitCallExpression(ast::CallExpression& expr) = 0;
  virtual void visitGetExpression(ast::GetExpression& expr) = 0;
  virtual void visitAssignExpression(ast::AssignExpression& expr) = 0;
};

class ExpressionCompiler {
 public:
  virtual pex::PexValue compile(const ast::LiteralExpression& expr) = 0;
  virtual pex::PexValue compile(const ast::IdentifierExpression& expr) = 0;
  virtual pex::PexValue compile(const ast::CallExpression& expr) = 0;
  virtual pex::PexValue compile(const ast::GetExpression& expr) = 0;
  virtual pex::PexValue compile(const ast::AssignExpression& expr) = 0;
};

}  // namespace ast
}  // namespace vellum