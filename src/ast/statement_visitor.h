#pragma once

namespace vellum {

namespace ast {

class ScriptStatement;

class StatementVisitor {
 public:
  virtual void visitScriptStatement(ScriptStatement& statement) = 0;
};

}  // namespace ast
}  // namespace vellum