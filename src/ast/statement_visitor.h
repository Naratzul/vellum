#pragma once

namespace vellum {

namespace ast {

class ScriptStatement;
class VariableDeclaration;

class StatementVisitor {
 public:
  virtual void visitScriptStatement(ScriptStatement& statement) = 0;
  virtual void visitVariableDeclaration(VariableDeclaration& statement) = 0;
};

}  // namespace ast
}  // namespace vellum