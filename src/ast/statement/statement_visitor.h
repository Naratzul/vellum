#pragma once

namespace vellum {

namespace ast {

class ExpressionStatement;
class ReturnStatement;
class IfStatement;
class LocalVariableStatement;

class StatementVisitor {
 public:
  virtual ~StatementVisitor() = default;
  virtual void visitExpressionStatement(ExpressionStatement& stmt) = 0;
  virtual void visitReturnStatement(ReturnStatement& stmt) = 0;
  virtual void visitIfStatement(IfStatement& stmt) = 0;
  virtual void visitLocalVariableStatement(LocalVariableStatement& stmt) = 0;
};

}  // namespace ast
}  // namespace vellum