#pragma once

namespace vellum {

namespace ast {

class ExpressionStatement;
class ReturnStatement;
class IfStatement;
class LocalVariableStatement;
class WhileStatement;
class BreakStatement;

class StatementVisitor {
 public:
  virtual ~StatementVisitor() = default;
  virtual void visitExpressionStatement(ExpressionStatement& statement) = 0;
  virtual void visitReturnStatement(ReturnStatement& statement) = 0;
  virtual void visitIfStatement(IfStatement& statement) = 0;
  virtual void visitLocalVariableStatement(
      LocalVariableStatement& statement) = 0;
  virtual void visitWhileStatement(WhileStatement& statement) = 0;
  virtual void visitBreakStatement(BreakStatement& statement) = 0;
};

}  // namespace ast
}  // namespace vellum